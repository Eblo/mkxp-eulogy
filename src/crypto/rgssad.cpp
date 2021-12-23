/*
** rgssad.cpp
**
** This file is part of mkxp.
**
** Copyright (C) 2014 Jonas Kulla <Nyocurio@gmail.com>
**
** mkxp is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 2 of the License, or
** (at your option) any later version.
**
** mkxp is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with mkxp.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "rgssad.h"
#include "boost-hash.h"

#include <regex>

struct BUGS_entryData
{
	int64_t offset;
	uint32_t patchVersion;
	uint32_t checksum;
	uint64_t size;
	uint32_t startMagic;
};

struct BUGS_entryHandle
{
	const BUGS_entryData data;
	uint32_t currentMagic;
	uint64_t currentOffset;
	PHYSFS_Io *io;
	uint32_t patchVersion;

	BUGS_entryHandle(const BUGS_entryData &data, PHYSFS_Io *archIo)
	    : data(data),
	      currentMagic(data.startMagic),
	      currentOffset(0),
		  patchVersion(data.patchVersion)
	{
		io = archIo->duplicate(archIo);
	}

	~BUGS_entryHandle()
	{
		io->destroy(io);
	}
};

struct BUGS_archiveData
{
	/* Maps: file path
	 * to:   entry data */
	BoostHash<std::string, BUGS_entryData> entryHash;

	/* Maps: directory path,
	 * to:   list of contained entries */
	BoostHash<std::string, BoostSet<std::string>> dirHash;
	
	const char* password;
	int passwordLength;
	int keyMultiplier;
	int keyAdditive;
	unsigned int keyIndex;
	std::regex patchMatcher;
};

struct BUGS_patchData
{
	PHYSFS_Io *archiveIo;
	uint32_t patchVersion;
	BUGS_archiveData *data;
};

/* Meta information shared between archives */
BUGS_archiveData *bugsMetaInformation;

static bool
readUint32(PHYSFS_Io *io, uint32_t &result)
{
	char buff[4];
	PHYSFS_sint64 count = io->read(io, buff, 4);

	result = ((buff[0] << 0x00) & 0x000000FF) |
	         ((buff[1] << 0x08) & 0x0000FF00) |
	         ((buff[2] << 0x10) & 0x00FF0000) |
	         ((buff[3] << 0x18) & 0xFF000000) ;

	return (count == 4);
}

#define PASSWORD_CHARACTER(i) bugsMetaInformation->password[i%bugsMetaInformation->passwordLength]
#define PHYSFS_ALLOC(type) \
	static_cast<type*>(PHYSFS_getAllocator()->Malloc(sizeof(type)))

static inline uint32_t
advanceMagic(uint32_t &magic)
{
	uint32_t old = magic;

	magic = magic * bugsMetaInformation->keyMultiplier + bugsMetaInformation->keyAdditive;

	return old;
}

static PHYSFS_sint64
BUGS_ioRead(PHYSFS_Io *self, void *buffer, PHYSFS_uint64 len)
{
	BUGS_entryHandle *entry = static_cast<BUGS_entryHandle*>(self->opaque);

	PHYSFS_Io *io = entry->io;

	uint64_t toRead = std::min<uint64_t>(entry->data.size - entry->currentOffset, len);
	uint64_t offs = entry->currentOffset;

	io->seek(io, entry->data.offset + offs);

	/* We divide up the bytes to be read in 3 categories:
	 *
	 * preAlign: If the current read address is not dword
	 *   aligned, this is the number of bytes to read til
	 *   we reach alignment again (therefore can only be
	 *   3 or less).
	 *
	 * align: The number of aligned dwords we can read
	 *   times 4 (= number of bytes).
	 *
	 * postAlign: The number of bytes to read after the
	 *   last aligned dword. Always 3 or less.
	 *
	 * Treating the pre- and post aligned reads specially,
	 * we can read all aligned dwords in one syscall directly
	 * into the write buffer and then run the xor chain on
	 * it afterwards. */

	uint8_t preAlign = 4 - (offs % 4);

	if (preAlign == 4)
		preAlign = 0;
	else
		preAlign = std::min<uint64_t>(preAlign, len);

	uint8_t postAlign = (len > preAlign) ? (offs + len) % 4 : 0;

	uint64_t align = len - (preAlign + postAlign);

	/* Byte buffer pointer */
	uint8_t *bBufferP = static_cast<uint8_t*>(buffer);

	if (preAlign > 0)
	{
		uint32_t dword;
		io->read(io, &dword, preAlign);

		/* Need to align the bytes with the
		 * magic before xoring */
		dword <<= 8 * (offs % 4);
		dword ^= entry->currentMagic;

		/* Shift them back to normal */
		dword >>= 8 * (offs % 4);
		memcpy(bBufferP, &dword, preAlign);

		bBufferP += preAlign;

		/* Only advance the magic if we actually
		 * reached the next alignment */
		if ((offs+preAlign) % 4 == 0)
			advanceMagic(entry->currentMagic);
	}

	if (align > 0)
	{
		/* Double word buffer pointer */
		uint32_t *dwBufferP = reinterpret_cast<uint32_t*>(bBufferP);

		/* Read aligned dwords in one go */
		io->read(io, bBufferP, align);

		/* Then xor them */
		for (uint64_t i = 0; i < (align / 4); ++i)
			dwBufferP[i] ^= advanceMagic(entry->currentMagic);

		bBufferP += align;
	}

	if (postAlign > 0)
	{
		uint32_t dword;
		io->read(io, &dword, postAlign);

		/* Bytes are already aligned with magic */
		dword ^= entry->currentMagic;
		memcpy(bBufferP, &dword, postAlign);
	}

	entry->currentOffset += toRead;

	return toRead;
}

static int
BUGS_ioSeek(PHYSFS_Io *self, PHYSFS_uint64 offset)
{
	BUGS_entryHandle *entry = static_cast<BUGS_entryHandle*>(self->opaque);

	if (offset == entry->currentOffset)
		return 1;

	if (offset > entry->data.size-1)
		return 0;

	/* If rewinding, we need to rewind to begining */
	if (offset < entry->currentOffset)
	{
		entry->currentOffset = 0;
		entry->currentMagic = entry->data.startMagic;
	}

	/* For each overstepped alignment, advance magic */
	uint64_t currentDword = entry->currentOffset / 4;
	uint64_t targetDword  = offset / 4;
	uint64_t dwordsSought = targetDword - currentDword;

	for (uint64_t i = 0; i < dwordsSought; ++i)
		advanceMagic(entry->currentMagic);

	entry->currentOffset = offset;
	entry->io->seek(entry->io, entry->data.offset + entry->currentOffset);

	return 1;
}

static PHYSFS_sint64
BUGS_ioTell(PHYSFS_Io *self)
{
	const BUGS_entryHandle *entry = static_cast<BUGS_entryHandle*>(self->opaque);

	return entry->currentOffset;
}

static PHYSFS_sint64
BUGS_ioLength(PHYSFS_Io *self)
{
	const BUGS_entryHandle *entry = static_cast<BUGS_entryHandle*>(self->opaque);

	return entry->data.size;
}

static PHYSFS_Io*
BUGS_ioDuplicate(PHYSFS_Io *self)
{
	const BUGS_entryHandle *entry = static_cast<BUGS_entryHandle*>(self->opaque);
	BUGS_entryHandle *entryDup = new BUGS_entryHandle(*entry);

	PHYSFS_Io *dup = PHYSFS_ALLOC(PHYSFS_Io);
	*dup = *self;
	dup->opaque = entryDup;

	return dup;
}

static void
BUGS_ioDestroy(PHYSFS_Io *self)
{
	BUGS_entryHandle *entry = static_cast<BUGS_entryHandle*>(self->opaque);

	delete entry;

	PHYSFS_getAllocator()->Free(self);
}

static const PHYSFS_Io BUGS_IoTemplate =
{
    0, /* version */
    0, /* opaque */
    BUGS_ioRead,
    0, /* write */
    BUGS_ioSeek,
    BUGS_ioTell,
    BUGS_ioLength,
    BUGS_ioDuplicate,
    0, /* flush */
    BUGS_ioDestroy
};

static void
processDirectories(BUGS_archiveData *data, BoostSet<std::string> &topLevel,
                   char *nameBuf, uint32_t nameLen)
{
	/* Check for top level entries */
	for (uint32_t i = 0; i < nameLen; ++i)
	{
		bool slash = nameBuf[i] == '/';
		if (!slash && i+1 < nameLen)
			continue;

		if (slash)
			nameBuf[i] = '\0';

		topLevel.insert(nameBuf);

		if (slash)
			nameBuf[i] = '/';

		break;
	}

	/* Check for more entries */
	for (uint32_t i = nameLen; i > 0; i--)
		if (nameBuf[i] == '/')
		{
			nameBuf[i] = '\0';

			const char *dir = nameBuf;
			const char *entry = &nameBuf[i+1];

			BoostSet<std::string> &entryList = data->dirHash[dir];
			entryList.insert(entry);
		}
}

static PHYSFS_EnumerateCallbackResult
BUGS_enumerateFiles(void *opaque, const char *dirname,
                    PHYSFS_EnumerateCallback cb,
                    const char *origdir, void *callbackdata)
{
	BUGS_archiveData *data = static_cast<BUGS_patchData*>(opaque)->data;

	std::string _dirname(dirname);

	if (!data->dirHash.contains(_dirname))
		return PHYSFS_ENUM_STOP;

	const BoostSet<std::string> &entries = data->dirHash[_dirname];

	BoostSet<std::string>::const_iterator iter;
	for (iter = entries.cbegin(); iter != entries.cend(); ++iter)
		cb(callbackdata, origdir, iter->c_str());

	return PHYSFS_ENUM_OK;
}

static PHYSFS_Io*
BUGS_openRead(void *opaque, const char *filename)
{
	BUGS_patchData *patchData = static_cast<BUGS_patchData*>(opaque);
	BUGS_archiveData *data = patchData->data;
	uint32_t patchVersion = patchData->patchVersion;


	if (!data->entryHash.contains(filename) ||
		data->entryHash.value(filename).patchVersion != patchVersion)
		return 0;

	BUGS_entryHandle *entry =
	        new BUGS_entryHandle(data->entryHash[filename], patchData->archiveIo);

	PHYSFS_Io *io = PHYSFS_ALLOC(PHYSFS_Io);

	*io = BUGS_IoTemplate;
	io->opaque = entry;

	return io;
}

static int
BUGS_stat(void *opaque, const char *filename, PHYSFS_Stat *stat)
{
	BUGS_archiveData *data = static_cast<BUGS_patchData*>(opaque)->data;

	bool hasFile = data->entryHash.contains(filename);
	bool hasDir  = data->dirHash.contains(filename);

	if (!hasFile && !hasDir)
	{
		PHYSFS_setErrorCode(PHYSFS_ERR_NOT_FOUND);
		return 0;
	}

	stat->modtime    =
	stat->createtime =
	stat->accesstime = 0;
	stat->readonly   = 1;

	if (hasFile)
	{
		const BUGS_entryData &entry = data->entryHash[filename];

		stat->filesize = entry.size;
		stat->filetype = PHYSFS_FILETYPE_REGULAR;
	}
	else
	{
		stat->filesize = 0;
		stat->filetype = PHYSFS_FILETYPE_DIRECTORY;
	}

	return 1;
}

static void
BUGS_closeArchive(void *opaque)
{
	BUGS_archiveData *data = static_cast<BUGS_patchData*>(opaque)->data;

	delete data;
}

static PHYSFS_Io*
RGSS_noop1(void*, const char*)
{
	return 0;
}

static int
RGSS_noop2(void*, const char*)
{
	return 0;
}



static bool
readUint32AndXor(PHYSFS_Io *io, uint32_t &result)
{
    result ^= bugsMetaInformation->password[0];
	if (!readUint32(io, result))
		return false;

    result ^= PASSWORD_CHARACTER(bugsMetaInformation->keyIndex++);

	return true;
}

static bool
decryptAndReadString(PHYSFS_Io *io, char* buffer, int stringLength)
{
	io->read(io, buffer, stringLength);

    for(int i=0; i < stringLength; i++) {
        buffer[i] ^= ((PASSWORD_CHARACTER(bugsMetaInformation->keyIndex++)));
    }
	buffer[stringLength] = '\0';
	return true;
}

static void*
BUGS_openArchive(PHYSFS_Io *io, const char *path, int forWrite, int *claimed)
{
	if (forWrite)
		return NULL;
	*claimed = 1;

	std::string patchName(path);
	std::smatch sm;
	std::regex_search(patchName, sm, bugsMetaInformation->patchMatcher);
	if (sm.size() != 1)
		return NULL;


	BUGS_patchData *patchData = new BUGS_patchData;
	patchData->patchVersion = std::stoi(sm[0]);
	patchData->data = bugsMetaInformation;
	patchData->archiveIo = io;

	return patchData;
}

void
BUGS_openMetaArchive(PHYSFS_Io *io, std::string password, int keyMultiplier, int keyAdditive)
{
	io->seek(io, 8);

	bugsMetaInformation = new BUGS_archiveData;
	bugsMetaInformation->password = password.c_str();
	bugsMetaInformation->passwordLength = password.length();
	bugsMetaInformation->keyMultiplier = keyMultiplier;
	bugsMetaInformation->keyAdditive = keyAdditive;
	bugsMetaInformation->keyIndex = -1;
	bugsMetaInformation->patchMatcher = std::regex("\\d+");

	/* Top level entry list */
	BoostSet<std::string> &topLevel = bugsMetaInformation->dirHash[""];

	uint32_t offset, patchVersion, checksum, fileSize, magicKey, fileNameLen;
	static char fileName[512];

	while (true)
	{
		if (!readUint32AndXor(io, offset))
			goto error;

		/* Zero offset means entry list has ended */
		if(offset <= 0)
			break;

		if (!readUint32AndXor(io, patchVersion))
			goto error;

		if (!readUint32AndXor(io, checksum))
			goto error;

		if (!readUint32AndXor(io, fileSize))
			goto error;

		if (!readUint32AndXor(io, magicKey))
			goto error;

		if (!readUint32AndXor(io, fileNameLen))
			goto error;
		if (!decryptAndReadString(io, fileName, fileNameLen))
			goto error;

		BUGS_entryData entry;
		entry.offset = offset;
		entry.size = fileSize;
		entry.startMagic = magicKey;
		entry.patchVersion = patchVersion;
		bugsMetaInformation->entryHash.insert(fileName, entry);
		processDirectories(bugsMetaInformation, topLevel, fileName, fileNameLen);

		continue;

	error:
		delete bugsMetaInformation;
		return;
	}

}

const PHYSFS_Archiver Bugs_Archiver =
{
	0,
	{
		"BUGS",
		"BUGS encrypted patch format",
		"", /* Author */
		"", /* Website */
		0 /* symlinks not supported */
	},
	BUGS_openArchive,
	BUGS_enumerateFiles,
	BUGS_openRead,
	RGSS_noop1, /* openWrite */
	RGSS_noop1, /* openAppend */
	RGSS_noop2, /* remove */
	RGSS_noop2, /* mkdir */
	BUGS_stat,
	BUGS_closeArchive
};
