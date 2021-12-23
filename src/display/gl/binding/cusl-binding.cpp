#ifdef MKXPZ_STEAM
#include "binding-util.h"
#include "steamshim_child.h"

#define STEAMSHIM_GETV(t, v, d)                                                \
  while (STEAMSHIM_alive()) {                                                  \
    const STEAMSHIM_Event *e = STEAMSHIM_pump();                               \
    if (e && e->type == t) {                                                   \
      d = e->v;                                                                \
      break;                                                                   \
    }                                                                          \
  }

#define STEAMSHIM_GETV_EXP(t, exp)                                             \
  while (STEAMSHIM_alive()) {                                                  \
    const STEAMSHIM_Event *e = STEAMSHIM_pump();                               \
    if (e && e->type == t) {                                                   \
      exp;                                                                     \
      break;                                                                   \
    }                                                                          \
  }

#define STEAMSHIM_GET_OK(t, d) STEAMSHIM_GETV(t, okay, d)

#define STEAMSHIM_GETV_AND_OK(t, v, dst, ok)                                   \
  while (STEAMSHIM_alive()) {                                                  \
    const STEAMSHIM_Event *e = STEAMSHIM_pump();                               \
    if (e && e->type == t) {                                                   \
      dst = e->v;                                                              \
      ok = e->okay;                                                            \
      break;                                                                   \
    }                                                                          \
  }

RB_METHOD(CUSLSetStat) {
  RB_UNUSED_PARAM;

  VALUE name, stat;
  rb_scan_args(argc, argv, "2", &name, &stat);
  SafeStringValue(name);

  bool ret;
  if (RB_TYPE_P(stat, RUBY_T_FLOAT)) {
    STEAMSHIM_setStatF(RSTRING_PTR(name), (float)RFLOAT_VALUE(stat));
    STEAMSHIM_GET_OK(SHIMEVENT_SETSTATF, ret);
  } else if (RB_TYPE_P(stat, RUBY_T_FIXNUM)) {
    STEAMSHIM_setStatI(RSTRING_PTR(name), (int)NUM2INT(stat));
    STEAMSHIM_GET_OK(SHIMEVENT_SETSTATI, ret);
  } else {
    rb_raise(rb_eTypeError,
             "Statistic value must be either an integer or float.");
  }
  return rb_bool_new(ret);
}

RB_METHOD(CUSLGetStatI) {
  RB_UNUSED_PARAM;

  VALUE name;
  rb_scan_args(argc, argv, "1", &name);
  SafeStringValue(name);

  int resi;
  bool valid;

  STEAMSHIM_getStatI(RSTRING_PTR(name));
  STEAMSHIM_GETV_AND_OK(SHIMEVENT_GETSTATI, ivalue, resi, valid);

  if (!valid)
    return Qnil;

  return INT2NUM(resi);
}

RB_METHOD(CUSLGetStatF) {
  RB_UNUSED_PARAM;

  VALUE name;
  rb_scan_args(argc, argv, "1", &name);
  SafeStringValue(name);

  float resf;
  bool valid;

  STEAMSHIM_getStatI(RSTRING_PTR(name));
  STEAMSHIM_GETV_AND_OK(SHIMEVENT_GETSTATI, fvalue, resf, valid);

  if (!valid)
    return Qnil;

  return rb_float_new(resf);
}

RB_METHOD(CUSLGetAchievement) {
  RB_UNUSED_PARAM;

  VALUE name;
  rb_scan_args(argc, argv, "1", &name);
  SafeStringValue(name);

  bool ret;
  bool valid;

  STEAMSHIM_getAchievement(RSTRING_PTR(name));
  STEAMSHIM_GETV_AND_OK(SHIMEVENT_GETACHIEVEMENT, ivalue, ret, valid);

  if (!valid)
    return Qnil;

  return rb_bool_new(ret);
}

RB_METHOD(CUSLSetAchievement) {
  RB_UNUSED_PARAM;

  VALUE name;
  rb_scan_args(argc, argv, "1", &name);
  SafeStringValue(name);

  bool ret;
  STEAMSHIM_setAchievement(RSTRING_PTR(name), true);
  STEAMSHIM_GET_OK(SHIMEVENT_SETACHIEVEMENT, ret);
  return rb_bool_new(ret);
}

RB_METHOD(CUSLClearAchievement) {
  RB_UNUSED_PARAM;

  VALUE name;

  rb_scan_args(argc, argv, "1", &name);
  SafeStringValue(name);

  bool ret;
  STEAMSHIM_setAchievement(RSTRING_PTR(name), false);
  STEAMSHIM_GET_OK(SHIMEVENT_SETACHIEVEMENT, ret);
  return rb_bool_new(ret);
}

RB_METHOD(CUSLGetAchievementAndUnlockTime) {
  RB_UNUSED_PARAM;

  VALUE name;

  rb_scan_args(argc, argv, "1", &name);
  SafeStringValue(name);

  bool achieved;
  unsigned long long time;
  bool valid;

  STEAMSHIM_getAchievement(RSTRING_PTR(name));
  STEAMSHIM_GETV_EXP(SHIMEVENT_GETACHIEVEMENT, {
    valid = e->okay;
    achieved = e->ivalue;
    time = e->epochsecs;
  });

  if (!valid)
    return Qnil;

  VALUE ret = rb_ary_new();
  rb_ary_push(ret, rb_bool_new(achieved));

  if (!time) {
    rb_ary_push(ret, Qnil);
    return ret;
  }

  VALUE t = rb_funcall(rb_cTime, rb_intern("at"), 1, ULL2NUM(time));
  rb_ary_push(ret, t);
  return ret;
}

RB_METHOD(CUSLStoreStats) {
  RB_UNUSED_PARAM;

  rb_check_argc(argc, 0);
  bool ok;

  STEAMSHIM_storeStats();
  STEAMSHIM_GET_OK(SHIMEVENT_STATSSTORED, ok);
  return rb_bool_new(ok);
}

RB_METHOD(CUSLResetAllStats) {
  RB_UNUSED_PARAM;

  bool achievementsToo;

  rb_get_args(argc, argv, "b", &achievementsToo);

  STEAMSHIM_resetStats(achievementsToo);
  STEAMSHIM_GET_OK(SHIMEVENT_RESETSTATS, achievementsToo);
  return rb_bool_new(achievementsToo);
}

void CUSLBindingInit() {

  STEAMSHIM_requestStats();
  bool ok;
  STEAMSHIM_GET_OK(SHIMEVENT_STATSRECEIVED, ok);

  VALUE mSteamLite = rb_define_module("SteamLite");

  _rb_define_module_function(mSteamLite, "get_stat_i", CUSLGetStatI);
  _rb_define_module_function(mSteamLite, "get_stat_f", CUSLGetStatF);
  _rb_define_module_function(mSteamLite, "set_stat", CUSLSetStat);

  _rb_define_module_function(mSteamLite, "store_stats", CUSLStoreStats);

  _rb_define_module_function(mSteamLite, "get_achievement", CUSLGetAchievement);
  _rb_define_module_function(mSteamLite, "set_achievement", CUSLSetAchievement);
  _rb_define_module_function(mSteamLite, "clear_achievement",
                             CUSLClearAchievement);
  _rb_define_module_function(mSteamLite, "get_achievement_and_unlock_time",
                             CUSLGetAchievementAndUnlockTime);

  _rb_define_module_function(mSteamLite, "reset_all_stats", CUSLResetAllStats);
}
#endif
