#include "rb_shader.h"
#include "glstate.h"
#include "gl-util.h"
#include "binding-util.h"
#include "sharedstate.h"
#include "shader.h"
#include "bitmap.h"

#ifndef MKXPZ_BUILD_XCODE
#include "rb_simple.vert.xxd"
#endif

#include <string>
#include <map>

#ifdef MKXPZ_BUILD_XCODE
#include "filesystem/filesystem.h"
CompiledShader::CompiledShader(const char *contents, VALUE args) : contents(contents),
                                                                   vertContents((const char *)mkxp_fs::contentsOfAssetAsString("Shaders/rb_simple", "vert").c_str())
#else
CompiledShader::CompiledShader(const char *contents, VALUE args) : contents(contents),
                                                                   vertContents((const char *)___shader_rb_simple_vert)
#endif
{
    fragShader = gl.CreateShader(GL_FRAGMENT_SHADER);
    vertShader = gl.CreateShader(GL_VERTEX_SHADER);

    program = gl.CreateProgram();

    compileShader(contents, fragShader, program, false);
    compileShader(vertContents, vertShader, program, true);
    setupArgs(args);

    ShaderBase::init();
}

CompiledShader::CompiledShader(const char *contents, VALUE args, const char *vertContents) : contents(contents),
                                                                                             vertContents(vertContents)
{
    fragShader = gl.CreateShader(GL_FRAGMENT_SHADER);
    vertShader = gl.CreateShader(GL_VERTEX_SHADER);

    program = gl.CreateProgram();

    compileShader(contents, fragShader, program, false);
    compileShader(vertContents, vertShader, program, true);
    setupArgs(args);

    ShaderBase::init();
}

std::string getShaderLog(GLuint shader)
{
    GLint logLength;
    gl.GetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);

    std::string log(logLength, '\0');
    gl.GetShaderInfoLog(shader, log.size(), 0, &log[0]);

    return log;
}

void CompiledShader::setupShaderSource(const char *contents, GLuint shader, bool vert)
{
    static const char glesDefine[] = "#define GLSLES\n";
    static const char fragDefine[] = "#define FRAGMENT_SHADER\n";

    const GLchar *shaderSrc[3];
    GLint shaderSrcSize[3];
    size_t i = 0;

    if (gl.glsles)
    {
        shaderSrc[i] = glesDefine;
        shaderSrcSize[i] = sizeof(glesDefine) - 1;
        ++i;
    }

    if (!vert)
    {
        shaderSrc[i] = fragDefine;
        shaderSrcSize[i] = sizeof(fragDefine) - 1;
        ++i;
    }

    shaderSrc[i] = (const GLchar *)contents;
    shaderSrcSize[i] = strlen(contents);
    ++i;

    gl.ShaderSource(shader, i, shaderSrc, shaderSrcSize);
}

void CompiledShader::compileShader(const char *contents, GLuint shader, GLuint program, bool vert)
{
    GLint success;

    setupShaderSource(contents, shader, vert);
    gl.CompileShader(shader);

    gl.GetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        rb_raise(rb_eRuntimeError, "Shader compilation failed with: \n%s", getShaderLog(shader).c_str());
        return;
    }

    gl.AttachShader(program, shader);

    gl.BindAttribLocation(program, Position, "position");
    gl.BindAttribLocation(program, TexCoord, "texCoord");
    gl.BindAttribLocation(program, Color, "color");

    // This conditional assumes that vertex shaders are handled after fragment shaders.
    // Alternatively, one may call LinkProgram() and do the error checking in every place where this compileShader()
    // method is called.
    if (vert) {
        gl.LinkProgram(program);
        gl.GetProgramiv(program, GL_LINK_STATUS, &success);

        if (!success)
        {
            rb_raise(rb_eRuntimeError, "Shader linking failed with: \n%s", getShaderLog(program).c_str());
            return;
        }
    }
}

void CompiledShader::setupArgs(VALUE pargs)
{
    long argc = rb_array_len(pargs);
    args = std::map<const char *, GLint>();

    for (long i = 0; i < argc; ++i)
    {
        VALUE arg = rb_ary_entry(pargs, i);
        const char *name = rb_string_value_cstr(&arg);
        args[name] = gl.GetUniformLocation(program, name);
    }
}

const char *CompiledShader::getContents()
{
    return contents;
}

const char *CompiledShader::getVertContents()
{
    return vertContents;
}

std::map<const char *, GLint> CompiledShader::getArgs()
{
    return args;
}

GLuint CompiledShader::getProgram()
{
    return program;
}

CustomShader::CustomShader(CompiledShader *shader, VALUE args, VALUE texUnits) : shader(shader),
                                                                                 args(args), texUnits(texUnits),
                                                                                 phase(0)
{
}

GLint CustomShader::getUniform(const char *name)
{
    return gl.GetUniformLocation(shader->getProgram(), name);
}


bool CustomShader::supportsVariable(const char *name)
{
    return getUniform(name) != -1;
}

void CustomShader::setMatrix4(const char *name, const float value[16])
{
    GLint u_name = getUniform(name);
    if(u_name != -1)
        gl.UniformMatrix4fv(u_name, 1, GL_FALSE, value);
}

void CustomShader::setVec4(const char *name, const Vec4 &value)
{
    GLint u_name = getUniform(name);
    if(u_name != -1)
        gl.Uniform4f(u_name, value.x, value.y, value.z, value.w);
}

void CustomShader::setFloat(const char *name, const float value)
{
    GLint u_name = getUniform(name);
    if(u_name != -1)
        gl.Uniform1f(u_name, value);
}

void CustomShader::setInteger(const char *name, const int value)
{
    GLint u_name = getUniform(name);
    if(u_name != -1)
        gl.Uniform1i(u_name, value);
}

/*
    Ideally, I would like these shaders to be based on time, but with so
    much of RM assuming 60 FPS, this will do for now.
*/
void CustomShader::incrementPhase()
{
    GLint u_phase = getUniform("phase");
    if(u_phase != -1)
	    gl.Uniform1i(u_phase, phase++);
}

/*
    Assuming a target frame rate of 60, we multiply the time by a constant to convert the current time in microseconds
    to an approximation of the number of frames that should have passed. This lets this time variable work in lieu of
    the phase variable above.
*/
void CustomShader::setTime()
{
    GLint u_time = getUniform("timeStep");
    if(u_time != -1)
	    gl.Uniform1i(u_time, shState->runTime() * 0.0000625);
}

#define IS_A(klass) if ( \
    rb_obj_is_kind_of(   \
        value,           \
        rb_path2class(klass)))

void CustomShader::applyArgs()
{
    std::map<const char *, GLint> args = shader->getArgs();
    std::map<const char *, GLint>::iterator it;
    for (it = args.begin(); it != args.end(); it++)
    {
        VALUE value = rb_hash_aref(this->args, rb_str_new_cstr(it->first));

        if (value == Qnil)
            rb_raise(rb_eIndexError, "No such argument: %s", it->first);

        if (it->second == -1)
            rb_raise(rb_eIndexError, "No such uniform declared in shader: %s", it->first);

        IS_A("Float")
        {
            gl.Uniform1f(it->second, NUM2DBL(value));
        }
        else IS_A("Integer")
        {
            gl.Uniform1i(it->second, NUM2INT(value));
        }
        else IS_A("Vec2")
        {
            Vec2 *vec2 = getPrivateData<Vec2>(value);
            gl.Uniform2f(it->second, vec2->x, vec2->y);
        }
        else IS_A("Vec4")
        {
            Vec4 *vec4 = getPrivateData<Vec4>(value);
            gl.Uniform4f(it->second, vec4->x, vec4->y, vec4->z, vec4->w);
        }
        else IS_A("Bitmap")
        {
            VALUE unitObj = rb_hash_fetch(texUnits, rb_str_new_cstr(it->first));

            if (unitObj == Qnil)
                rb_raise(rb_eIndexError, "No such texture unit: %s, please supply this when creating the shader", it->first);

            unsigned unitNum = NUM2UINT(unitObj);
            Bitmap *bitmap = getPrivateData<Bitmap>(value);

            GLenum texUnit = GL_TEXTURE0 + unitNum;

            gl.ActiveTexture(texUnit);
            gl.BindTexture(GL_TEXTURE_2D, bitmap->getGLTypes().tex.gl);
            gl.Uniform1i(it->second, unitNum);
            gl.ActiveTexture(GL_TEXTURE0);
        }
        else
        {
            rb_raise(rb_eArgError, "Argument %s is of type %s, which is not a supported type", it->first, rb_obj_classname(value));
        }
    }
}

CompiledShader *CustomShader::getShader()
{
    return shader;
}
