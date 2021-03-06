/* --------------------------------------------------------------------------------
 #
 #	4DPlugin-Appointments.h
 #	source generated by 4D Plugin Wizard
 #	Project : Appointments
 #	author : miyako
 #	2020/09/06
 #  
 # --------------------------------------------------------------------------------*/

#ifndef PLUGIN_APPOINTMENTS_H
#define PLUGIN_APPOINTMENTS_H

#include "4DPluginAPI.h"
#include "ARRAY_TEXT.h"
#include "C_TEXT.h"
#include "4DPlugin-JSON.h"

#define DATE_FORMAT_ISO @"yyyy-MM-dd'T'HH:mm:ss"
#define DATE_FORMAT_ISO_GMT @"yyyy-MM-dd'T'HH:mm:ss'Z'"

#import <CalendarStore/CalendarStore.h>

#include "json/json.h"

#include "sqlite3.h"

#import <EventKit/EventKit.h>
#import <Security/Security.h>

#include <map>

typedef enum {
    
    request_permission_unknown = 0,
    request_permission_authorized = 1,
    request_permission_not_determined = 2,
    request_permission_denied = 3,
    request_permission_restricted = 4
    
}request_permission_t;

typedef enum
{
    
    notification_create = 0,
    notification_update = 1,
    notification_delete = 2
    
} notification_t;

void listenerInit(void);
void listenerLoop(void);
void listenerLoopStart(void);
void listenerLoopFinish(void);
void listenerLoopExecute(void);
void listenerLoopExecuteMethod(void);

typedef PA_long32 process_number_t;
typedef PA_long32 process_stack_size_t;
typedef PA_long32 method_id_t;
typedef PA_Unichar* process_name_t;

#pragma mark -

void ALL_APPOINTMENTS(PA_PluginParameters params);
void Get_appointment(PA_PluginParameters params);
void UPDATE_APPOINTMENT(PA_PluginParameters params);
void DELETE_APPOINTMENT(PA_PluginParameters params);
void ON_APPOINTMENT_CALL(PA_PluginParameters params);
void CREATE_APPOINTMENT(PA_PluginParameters params);
void APPOINTMENT_NAMES(PA_PluginParameters params);

#endif /* PLUGIN_APPOINTMENTS_H */
