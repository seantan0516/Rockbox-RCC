/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 by Thomas Martitz
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/


#include <jni.h>
#include <stdbool.h>
#include <stddef.h>
#include "config.h"

extern JNIEnv* getJavaEnvironment(void);

static jfieldID __battery_level;
static jobject BatteryMonitor_instance;

/* Called from Java before Rockbox's native main loop starts. Doing class
 * lookup and object construction from _battery_level() used to run JNI
 * initialization deep inside the native startup/render call stack. */
JNIEXPORT void JNICALL
Java_org_rockbox_RockboxService_initBatteryMonitor(JNIEnv *env, jobject service,
                                                   jobject monitor)
{
    (void)service;
    jclass cls = (*env)->GetObjectClass(env, monitor);
    if (cls == NULL || (*env)->ExceptionCheck(env))
    {
        if ((*env)->ExceptionCheck(env))
            (*env)->ExceptionClear(env);
        return;
    }

    jfieldID field = (*env)->GetFieldID(env, cls, "mBattLevel", "I");
    if (field == NULL || (*env)->ExceptionCheck(env))
    {
        if ((*env)->ExceptionCheck(env))
            (*env)->ExceptionClear(env);
        (*env)->DeleteLocalRef(env, cls);
        return;
    }

    jobject global = (*env)->NewGlobalRef(env, monitor);
    if (global == NULL || (*env)->ExceptionCheck(env))
    {
        if ((*env)->ExceptionCheck(env))
            (*env)->ExceptionClear(env);
        (*env)->DeleteLocalRef(env, cls);
        return;
    }

    if (BatteryMonitor_instance != NULL)
        (*env)->DeleteGlobalRef(env, BatteryMonitor_instance);

    BatteryMonitor_instance = global;
    __battery_level = field;
    (*env)->DeleteLocalRef(env, cls);
}

int _battery_level(void)
{
    JNIEnv *env = getJavaEnvironment();

    /* Some Rockbox worker pthreads are not attached to the JVM. Battery
     * information is optional there; never reuse another thread's JNIEnv. */
    if (env == NULL || BatteryMonitor_instance == NULL || __battery_level == NULL)
        return -1;

    jint level = (*env)->GetIntField(env, BatteryMonitor_instance, __battery_level);
    if ((*env)->ExceptionCheck(env))
    {
        (*env)->ExceptionClear(env);
        return -1;
    }

    return level;
}

