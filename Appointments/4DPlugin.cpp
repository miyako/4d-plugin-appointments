/* --------------------------------------------------------------------------------
 #
 #	4DPlugin.cpp
 #	source generated by 4D Plugin Wizard
 #	Project : Appointments
 #	author : miyako
 #	2017/04/03
 #
 # --------------------------------------------------------------------------------*/


#include "4DPluginAPI.h"
#include "4DPlugin.h"

#define DATE_FORMAT_ISO @"yyyy-MM-dd'T'HH:mm:ss"
#define DATE_FORMAT_ISO_GMT @"yyyy-MM-dd'T'HH:mm:ss'Z'"

//typedef struct
//{
//	C_TEXT _method;
//	PA_long32 method_id;
//	const PA_Unichar *method;
//	size_t method_size;
//}appointment_ctx;
//
//appointment_ctx APPOINTMENT_CTX;

namespace DateFormatter
{
	NSDateFormatter *ISO;
	NSDateFormatter *GMT;
}

namespace CalendarWatch
{
	
	class UserInfo
	{
	private:
		
		notification_t _notification;
		CUTF8String _uid;
		method_id_t _method;
		
	public:
		
		UserInfo(notification_t notification, NSString *event_uid, method_id_t method);
		
		void get(notification_t *notification, CUTF16String &event, method_id_t *method);
		
		~UserInfo();
	};
	
	UserInfo::UserInfo(notification_t notification, NSString *event_uid, method_id_t method)
	{
		this->_notification = notification;
		if(event_uid)
		{
			this->_uid = CUTF8String((const uint8_t *)[event_uid UTF8String]);
		}
		this->_method = method;
	}
	
	void UserInfo::get(notification_t *notification, CUTF16String &event, method_id_t *method)
	{
		*notification = this->_notification;
		
		*method = this->_method;
		
		C_TEXT t;
		t.setUTF8String(&this->_uid);
		t.copyUTF16String(&event);
	}
	
	UserInfo::~UserInfo()
	{
		
	}
	
	const process_stack_size_t stachSize = 0;
	const process_name_t processName = (PA_Unichar *)"$\0C\0A\0L\0E\0N\0D\0A\0R\0_\0W\0A\0T\0C\0H\0\0\0";
	
	std::map<CUTF8String, method_id_t> paths;
	std::vector<UserInfo> notifications;
	
	FSEventStreamRef eventStream = 0;
	NSTimeInterval latency = 1.0;
	process_number_t monitorProcessId = 0;
	bool processShouldTerminate = false;
	
	method_id_t getMethodId(NSString *path_ns)
	{
		@autoreleasepool
		{
			NSString *monitorPath_ns = [path_ns stringByDeletingLastPathComponent];
			CUTF8String monitorPath = CUTF8String((const uint8_t *)[monitorPath_ns UTF8String]);
			monitorPath += (const uint8_t *)"/";
			//global variables: CalendarWatch::paths
			NSLock *l = [[NSLock alloc]init];
			if ([l tryLock])
			{
				auto i = paths.find(monitorPath);
				if (i != paths.end())
				{
					return i->second;
				}
				[l unlock];
			}
			[l release];
		}
		return 0;
	}
	
	void gotEvent(FSEventStreamRef stream,
								void *callbackInfo,
								size_t numEvents,
								void *eventPaths,
								const FSEventStreamEventFlags eventFlags[],
								const FSEventStreamEventId eventIds[]
								)
	{
		//global variables: CalendarWatch::notifications
		NSLock *l = [[NSLock alloc]init];
		if ([l tryLock])
		{
			@autoreleasepool
			{
				NSArray *paths_ns = (NSArray *)eventPaths;
				NSRegularExpression *regex = [NSRegularExpression regularExpressionWithPattern:@"([:HexDigit:]{8}-[:HexDigit:]{4}-[:HexDigit:]{4}-[:HexDigit:]{4}-[:HexDigit:]{12})\\.ics$"
																																							 options:NSRegularExpressionCaseInsensitive
																																								 error:nil];
				if(regex)
				{
					for(NSUInteger i = 0; i < numEvents; ++i)
					{
						NSString *path_ns = [paths_ns objectAtIndex:i];
						
						NSArray *matches = [regex matchesInString:path_ns
																							options:0
																								range:NSMakeRange(0, [path_ns length])];
						
						for (NSTextCheckingResult *match in matches)
						{
							NSString *event_uid = [path_ns substringWithRange:[match rangeAtIndex:1]];
							FSEventStreamEventFlags flags = eventFlags[i];
							method_id_t methodId = getMethodId(path_ns);
							NSLog(@"flags:%d", (unsigned int)flags);
							if(methodId)
							{
								if((flags & kFSEventStreamEventFlagItemRemoved) == kFSEventStreamEventFlagItemRemoved)
								{
									NSLog(@"removed calendar item:\t%@", event_uid);
									UserInfo userInfo(notification_delete, event_uid, methodId);
									notifications.push_back(userInfo);
								}
								else if((flags & kFSEventStreamEventFlagItemCreated) == kFSEventStreamEventFlagItemCreated)
								{
									NSLog(@"created calendar item:\t%@", event_uid);
									UserInfo userInfo(notification_create, event_uid, methodId);
									notifications.push_back(userInfo);
								}
								else
								{
									NSLog(@"modified calendar item:\t%@", event_uid);
									UserInfo userInfo(notification_update, event_uid, methodId);
									notifications.push_back(userInfo);
								}
							}
						}
					}
				}
			}//@autoreleasepool
			listenerLoopExecute();
			[l unlock];
		}
		[l release];
	}
	
	void endMonitor()
	{
		//global variables: CalendarWatch::eventStream
		NSLock *l = [[NSLock alloc]init];
		if ([l tryLock])
		{
			if(eventStream)
			{
				FSEventStreamStop(eventStream);
				FSEventStreamUnscheduleFromRunLoop (eventStream, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
				FSEventStreamInvalidate(eventStream);
				FSEventStreamRelease(eventStream);
				eventStream = 0;
				NSLog(@"stop monitoring paths");
			}
			[l unlock];
		}
		[l release];
	}
	
	void startMonitor()
	{
		FSEventStreamContext context = {0, NULL, NULL, NULL, NULL};
		
		NSMutableArray *paths_ns = [[NSMutableArray alloc]init];
		//global variables: CalendarWatch::paths
		NSLock *l = [[NSLock alloc]init];
		if ([l tryLock])
		{
			for(std::map<CUTF8String, method_id_t>::iterator it = CalendarWatch::paths.begin(); it != CalendarWatch::paths.end(); it++)
			{
				CUTF8String path = it->first;
				NSString *path_ns = [[NSString alloc]initWithUTF8String:(const char *)path.c_str()];
				if(path_ns)
				{
					[paths_ns addObject:path_ns];
					[path_ns release];
				}
			}
			[l unlock];
		}
		
		endMonitor();
		
		listenerLoopStart();
		
		if([paths_ns count])
		{
			if ([l tryLock])
			{
				eventStream = FSEventStreamCreate(NULL,
																					(FSEventStreamCallback)gotEvent,
																					&context,
																					(CFArrayRef)paths_ns,
																					kFSEventStreamEventIdSinceNow,
																					(CFAbsoluteTime)latency,
																					kFSEventStreamCreateFlagUseCFTypes
																					| kFSEventStreamCreateFlagFileEvents
																					| kFSEventStreamCreateFlagNoDefer
																					| kFSEventStreamCreateFlagIgnoreSelf
																					);
				NSLog(@"start monitoring paths:%@", [paths_ns description]);
				FSEventStreamScheduleWithRunLoop(eventStream, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
				FSEventStreamStart(eventStream);
				
				[l unlock];
			}
			[l release];
			
		}
		[paths_ns release];
	}
	
	bool isInWatch(CUTF8String &path)
	{
		bool inWatch = false;
		
		NSLock *l = [[NSLock alloc]init];
		
		if ([l tryLock])
		{
			auto i = paths.find(path);
			inWatch = (i != paths.end());
			[l unlock];
		}
		
		[l release];
		
		return inWatch;
	}
	
	void addToWatch(CUTF8String &path, method_id_t methodId)
	{
		if (!isInWatch(path))
		{
			NSLock *l = [[NSLock alloc]init];
			if ([l tryLock])
			{
				paths.insert(std::map<CUTF8String, method_id_t>::value_type(path, methodId));
				PA_RunInMainProcess((PA_RunInMainProcessProcPtr)startMonitor, NULL);
				[l unlock];
			}
			[l release];
		}
	}
	
	void removeFromWatch(CUTF8String &path)
	{
		NSLock *l = [[NSLock alloc]init];
		if ([l tryLock])
		{
			paths.erase(path);
			PA_RunInMainProcess((PA_RunInMainProcessProcPtr)startMonitor, NULL);
			if(!paths.size())
			{
				listenerLoopFinish();
			}
			[l unlock];
		}
		[l release];
	}
	
	void removeAllFromWatch()
	{
		NSLock *l = [[NSLock alloc]init];
		if ([l tryLock])
		{
			paths.clear();
			PA_RunInMainProcess((PA_RunInMainProcessProcPtr)endMonitor, NULL);
			listenerLoopFinish();
			[l unlock];
		}
		[l release];
	}
	
}

#pragma mark -

bool isProcessOnExit()
{
	C_TEXT name;
	PA_long32 state, time;
	PA_GetProcessInfo(PA_GetCurrentProcessNumber(), name, &state, &time);
	CUTF16String procName(name.getUTF16StringPtr());
	CUTF16String exitProcName((PA_Unichar *)"$\0x\0x\0\0\0");
	return (!procName.compare(exitProcName));
}

void onCloseProcess()
{
	if(isProcessOnExit())
	{
		CalendarWatch::removeAllFromWatch();
		[DateFormatter::GMT release];
		[DateFormatter::ISO release];
	}
}

void onStartup()
{
	DateFormatter::GMT = [[NSDateFormatter alloc]init];
	[DateFormatter::GMT setDateFormat:DATE_FORMAT_ISO_GMT];
	[DateFormatter::GMT setTimeZone:[NSTimeZone timeZoneForSecondsFromGMT:0]];
	
	DateFormatter::ISO = [[NSDateFormatter alloc]init];
	[DateFormatter::ISO setDateFormat:DATE_FORMAT_ISO];
	[DateFormatter::ISO setTimeZone:[NSTimeZone localTimeZone]];
}

#pragma mark -

void generateUuid(C_TEXT &returnValue)
{
#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 1080
	returnValue.setUTF16String([[[NSUUID UUID]UUIDString]stringByReplacingOccurrencesOfString:@"-" withString:@""]);
#else
	CFUUIDRef uuid = CFUUIDCreate(kCFAllocatorDefault);
	NSString *uuid_str = (NSString *)CFUUIDCreateString(kCFAllocatorDefault, uuid);
	returnValue.setUTF16String([uuid_str stringByReplacingOccurrencesOfString:@"-" withString:@""]);
#endif
}

void listenerLoop()
{
	CalendarWatch::monitorProcessId = PA_GetCurrentProcessNumber();
	NSLog(@"%@", @"listenerLoop:start");
	
	NSLock *l = [[NSLock alloc]init];
	
	while((!CalendarWatch::processShouldTerminate) && !PA_IsProcessDying())
	{
		PA_FreezeProcess(PA_GetCurrentProcessNumber());
		PA_YieldAbsolute();
		
		//global variables: CalendarWatch::notifications,processShouldTerminate
		if ([l tryLock])
		{
			if(!CalendarWatch::processShouldTerminate)
			{
				if(CalendarWatch::notifications.size())
				{
					C_TEXT processName;
					generateUuid(processName);
					PA_NewProcess((void *)listenerLoopExecuteMethod,
												CalendarWatch::stachSize,
												(PA_Unichar *)processName.getUTF16StringPtr());
				}
			}
			[l unlock];
		}
	}//while(!CalendarWatch::processShouldTerminate)
	CalendarWatch::monitorProcessId = 0;
	
	PA_KillProcess();
	NSLog(@"%@", @"listenerLoop:end");
	[l release];
}

void listenerLoopStart()
{
	//global variables: CalendarWatch::monitorProcessId,processShouldTerminate
	NSLock *l = [[NSLock alloc]init];
	if ([l tryLock])
	{
		if(!CalendarWatch::monitorProcessId)
		{
			CalendarWatch::processShouldTerminate = false;
			PA_NewProcess((void *)listenerLoop,
										CalendarWatch::stachSize,
										CalendarWatch::processName);
		}
		[l unlock];
	}
	[l release];
}

void listenerLoopFinish()
{
	//global variables: CalendarWatch::monitorProcessId,processShouldTerminate
	NSLock *l = [[NSLock alloc]init];
	if ([l tryLock])
	{
		CalendarWatch::processShouldTerminate = true;
		PA_UnfreezeProcess(CalendarWatch::monitorProcessId);
		[l unlock];
	}
	[l release];
}

void listenerLoopExecute()
{
	//global variables: CalendarWatch::monitorProcessId,processShouldTerminate
	NSLock *l = [[NSLock alloc]init];
	if ([l tryLock])
	{
		CalendarWatch::processShouldTerminate = false;
		PA_UnfreezeProcess(CalendarWatch::monitorProcessId);
		[l unlock];
	}
	[l release];
}

void listenerLoopExecuteMethod()
{
	//global variables: CalendarWatch::notifications
	NSLock *l = [[NSLock alloc]init];
	if ([l tryLock])
	{
		if(CalendarWatch::notifications.size())
		{
			std::vector<CalendarWatch::UserInfo>::iterator it = CalendarWatch::notifications.begin();
			
			CalendarWatch::UserInfo userInfo = *it;
			
			notification_t notification;
			CUTF16String eventId;
			method_id_t methodId;
			
			userInfo.get(&notification, eventId, &methodId);
			
			PA_Variable	params[3];

			params[0] = PA_CreateVariable(eVK_Longint);
			params[1] = PA_CreateVariable(eVK_Unistring);
			params[2] = PA_CreateVariable(eVK_Unistring);

			PA_SetLongintVariable(&params[0], notification);
			
			PA_Unistring event = PA_CreateUnistring((PA_Unichar *)eventId.c_str());
			PA_SetStringVariable(&params[1], &event);

			//eventId to JSON
			CUTF16String eventJson;
			event_to_json(&eventId, &eventJson);
			PA_Unistring eventInfo = PA_CreateUnistring((PA_Unichar *)eventJson.c_str());
			PA_SetStringVariable(&params[2], &eventInfo);
			
			CalendarWatch::notifications.erase(it);
			
			PA_ExecuteMethodByID(methodId, params, 3);
			
			PA_ClearVariable(&params[0]);
			PA_ClearVariable(&params[1]);
			PA_ClearVariable(&params[2]);
		}
		[l unlock];
	}
	[l release];
}

#pragma mark -
void PluginMain(PA_long32 selector, PA_PluginParameters params)
{
	try
	{
		PA_long32 pProcNum = selector;
		sLONG_PTR *pResult = (sLONG_PTR *)params->fResult;
		PackagePtr pParams = (PackagePtr)params->fParameters;
		
		CommandDispatcher(pProcNum, pResult, pParams);
	}
	catch(...)
	{
		
	}
}

void CommandDispatcher (PA_long32 pProcNum, sLONG_PTR *pResult, PackagePtr pParams)
{
	switch(pProcNum)
	{
		case kInitPlugin :
		case kServerInitPlugin :
			onStartup();
			break;
		case kCloseProcess :
			onCloseProcess();
			break;
			// --- Appointments
			
		case 1 :
			ALL_APPOINTMENTS(pResult, pParams);
			break;
			
		case 2 :
			Get_appointment(pResult, pParams);
			break;
			
		case 3 :
			UPDATE_APPOINTMENT(pResult, pParams);
			break;
			
		case 4 :
			DELETE_APPOINTMENT(pResult, pParams);
			break;
			
		case 5 :
			ON_APPOINTMENT_CALL(pResult, pParams);
			break;
			
		case 6 :
			CREATE_APPOINTMENT(pResult, pParams);
			break;
			
		case 7 :
			APPOINTMENT_NAMES(pResult, pParams);
			break;
	}
}
// --------------------------------- Appointments ---------------------------------

#pragma mark -

#pragma mark -

const char *sql_get_calendar_group_uid = "SELECT\n\
ZUID\n\
FROM ZNODE\n\
WHERE Z_PK ==\n\
(\n\
SELECT ZGROUP\n\
FROM  ZNODE\n\
WHERE ZUID == ?\n\
LIMIT 1\n\
);";

const char *sql_get_calendars = "SELECT\n\
ZUID, ZTITLE\n\
FROM ZNODE\n\
WHERE ZISTASKCONTAINER != 1\n\
AND ZGROUP != '';";

void sqlite3_get_calendar_group_uid(NSString *userCalendarPath,
																		CUTF8String &calendar_uid,
																		CUTF8String &group_uid)
{
	NSLock *l = [[NSLock alloc]init];
	if ([l tryLock])
	{
		sqlite3 *sqlite3_calendar = NULL;
		
		int err = sqlite3_open([userCalendarPath UTF8String], &sqlite3_calendar);
		
		if(err != SQLITE_OK)
		{
			NSLog(@"failed to open sqlite database at:%@", userCalendarPath);
		}else
		{
			sqlite3_stmt *sql = NULL;
			err = sqlite3_prepare_v2(sqlite3_calendar, sql_get_calendar_group_uid, 1024, &sql, NULL);
			if(err != SQLITE_OK)
			{
				NSLog(@"failed to prepare sqlite statement");
			}else
			{
				sqlite3_bind_text(sql, 1, (const char *)calendar_uid.c_str(), calendar_uid.length(), NULL);
				
				while(SQLITE_ROW == (err = sqlite3_step(sql)))
				{
					const unsigned char *_group_uid = sqlite3_column_text(sql, 0);
					if(_group_uid)
					{
						group_uid = CUTF8String(_group_uid, strlen((const char *)_group_uid));
					}
				}
				sqlite3_finalize(sql);
			}
			sqlite3_close(sqlite3_calendar);
		}
		[l unlock];
	}
	[l release];
}

void sqlite3_get_calendars(NSString *userCalendarPath,
													 ARRAY_TEXT &uids,
													 ARRAY_TEXT &titles)
{
	NSLock *l = [[NSLock alloc]init];
	if ([l tryLock])
	{
		sqlite3 *sqlite3_calendar = NULL;
		
		int err = sqlite3_open([userCalendarPath UTF8String], &sqlite3_calendar);
		
		if(err != SQLITE_OK)
		{
			NSLog(@"failed to open sqlite database at:%@", userCalendarPath);
		}else
		{
			sqlite3_stmt *sql = NULL;
			err = sqlite3_prepare_v2(sqlite3_calendar, sql_get_calendars, 1024, &sql, NULL);
			if(err != SQLITE_OK)
			{
				NSLog(@"failed to prepare sqlite statement");
			}else
			{
				while(SQLITE_ROW == (err = sqlite3_step(sql)))
				{
					const unsigned char *_calendar_uid = sqlite3_column_text(sql, 0);
					const unsigned char *_title = sqlite3_column_text(sql, 1);
					
					if(_calendar_uid)
					{
						if(_title)
						{
							uids.appendUTF8String(_calendar_uid, strlen((const char *)_calendar_uid));
							titles.appendUTF8String(_title, strlen((const char *)_title));
						}
						
					}
				}
				sqlite3_finalize(sql);
			}
			sqlite3_close(sqlite3_calendar);
		}
		[l unlock];
	}
	[l release];
}

#pragma mark -

void Get_appointment(sLONG_PTR *pResult, PackagePtr pParams)
{
	C_TEXT Param1;
	C_TEXT Param2;
	C_TEXT returnValue;
	
	Param1.fromParamAtIndex(pParams, 1);
	
	CUTF8String json;
	JSONNODE *r = json_new(JSON_NODE);
	
	if(r)
	{
		CalCalendarStore *defaultCalendarStore = json_get_calendar_store(r);
		
		if(defaultCalendarStore)
		{
			CalEvent *event = json_get_event(r, Param1);
			
			if(event)
			{
				JSONNODE *e = json_new(JSON_NODE);
				json_set_event(e, event);
				json_stringify(e, returnValue);
				json_delete(e);
			}
		}
		
		json_stringify(r, Param2);
		json_delete(r);
	}
	
	Param2.toParamAtIndex(pParams, 2);
	returnValue.setReturn(pResult);
}

void DELETE_APPOINTMENT(sLONG_PTR *pResult, PackagePtr pParams)
{
	C_TEXT Param1;
	C_TEXT Param2;
	
	Param1.fromParamAtIndex(pParams, 1);
	
	CUTF8String json;
	JSONNODE *r = json_new(JSON_NODE);
	
	if(r)
	{
		CalCalendarStore *defaultCalendarStore = json_get_calendar_store(r);
		
		if(defaultCalendarStore)
		{
			CalEvent *event = json_get_event(r, Param1);
			
			if(event)
			{
				NSError *err;
				if(![defaultCalendarStore removeEvent:event span:CalSpanAllEvents error:&err])
				{
					json_set_text(r, L"removeEvent", @"remove failed");
					json_set_text(r, L"removeEventErrorDescription", [err description]);
				}else
				{
					//OK
					json_set_text(r, L"removeEvent", @"OK");
				}
			}
		}
		json_stringify(r, Param2);
		json_delete(r);
	}
	
	Param2.toParamAtIndex(pParams, 2);
}

void UPDATE_APPOINTMENT(sLONG_PTR *pResult, PackagePtr pParams)
{
	C_TEXT Param1;
	C_TEXT Param2;
	C_TEXT Param3;
	
	Param1.fromParamAtIndex(pParams, 1);
	Param2.fromParamAtIndex(pParams, 2);
	
	JSONNODE *ee = json_parse(Param2);
	JSONNODE *r = json_new(JSON_NODE);
	
	if(r)
	{
		if(ee)
		{
			JSONNODE *e = ee;
			if(json_type(ee) == JSON_ARRAY)
			{
				if(json_size(ee))
				{
					e = json_at(ee, 0);
				}
			}
			
			CalCalendarStore *defaultCalendarStore = json_get_calendar_store(r);
			
			if(defaultCalendarStore)
			{
				CalEvent *event = json_get_event(r, Param1);
				
				if(event)
				{
					event.location = json_get_text(e, L"location");
					event.notes = json_get_text(e, L"notes");
					event.title = json_get_text(e, L"title");
					event.url = json_get_url(e, L"url");
					event.startDate = json_get_date(e, L"startDate");
					event.endDate = json_get_date(e, L"endDate");
					NSError *err = nil;
					if(![defaultCalendarStore saveEvent:event span:CalSpanThisEvent error:&err])
					{
						json_set_text(r, L"saveEvent", @"save failed");
						json_set_text(r, L"saveEventErrorDescription", [err description]);
					}else{
						//OK
						json_set_text(r, L"saveEvent", @"OK");
						json_set_event(e, event);
					}
				}
			}
			json_stringify(e, Param2);
			json_delete(ee);
		}
		json_stringify(r, Param3);
		json_delete(r);
	}
	Param2.toParamAtIndex(pParams, 2);
	Param3.toParamAtIndex(pParams, 3);
}

void CREATE_APPOINTMENT(sLONG_PTR *pResult, PackagePtr pParams)
{
	C_TEXT Param1;
	C_TEXT Param2;
	C_TEXT Param3;
	
	Param1.fromParamAtIndex(pParams, 1);
	Param2.fromParamAtIndex(pParams, 2);
	
	JSONNODE *ee = json_parse(Param2);
	JSONNODE *r = json_new(JSON_NODE);
	
	if(r)
	{
		if(ee)
		{
			JSONNODE *e = ee;
			if(json_type(ee) == JSON_ARRAY)
			{
				if(json_size(ee))
				{
					e = json_at(ee, 0);
				}
			}
			
			CalCalendarStore *defaultCalendarStore = json_get_calendar_store(r);
			
			if(defaultCalendarStore)
			{
				CalCalendar *calendar = json_get_calendar(r, Param1);
				
				if(calendar)
				{
					CalEvent *event = [CalEvent event];
					event.calendar = calendar;
					event.location = json_get_text(e, L"location");
					event.notes = json_get_text(e, L"notes");
					event.title = json_get_text(e, L"title");
					event.url = json_get_url(e, L"url");
					event.startDate = json_get_date(e, L"startDate");
					event.endDate = json_get_date(e, L"endDate");
					NSError *err = nil;
					if(![defaultCalendarStore saveEvent:event span:CalSpanThisEvent error:&err])
					{
						json_set_text(r, L"saveEvent", @"save failed");
						json_set_text(r, L"saveEventErrorDescription", [err description]);
					}else{
						//OK
						json_set_text(r, L"saveEvent", @"OK");
						json_set_event(e, event);
					}
				}
			}
			json_stringify(e, Param2);
			json_delete(ee);
		}
		json_stringify(r, Param3);
		json_delete(r);
	}
	Param2.toParamAtIndex(pParams, 2);
	Param3.toParamAtIndex(pParams, 3);
}

bool json_get_calendarPath(C_TEXT &Param1, CUTF8String &path)
{
	bool success = NO;
	
	NSString *userCalendarPath = [NSString stringWithFormat:@"%@/Calendars/",
																[NSSearchPathForDirectoriesInDomains(NSLibraryDirectory, NSUserDomainMask, YES)
																 objectAtIndex:0]];
	ARRAY_TEXT out_uids;
	ARRAY_TEXT out_titles;
	
	NSString *titleToSearch = Param1.copyUTF16String();
	
	sqlite3_get_calendars([NSString stringWithFormat:@"%@Calendar Cache", userCalendarPath], out_uids, out_titles);
	
	NSUInteger size = out_uids.getSize();
	
	for(NSUInteger i = 0; i < size; ++i)
	{
		NSString *title = out_titles.copyUTF16StringAtIndex(i);
		
		if([title isEqualToString:titleToSearch])
		{
			CUTF8String calendar_uid, group_uid;
			out_uids.copyUTF8StringAtIndex(&calendar_uid, i);
			
			sqlite3_get_calendar_group_uid([NSString stringWithFormat:@"%@Calendar Cache", userCalendarPath], calendar_uid, group_uid);
			
			NSFileManager *defaultManager = [[NSFileManager alloc]init];
			
			NSString *path_caldav = [NSString stringWithFormat:@"%@%s%s/%s%s", userCalendarPath, group_uid.c_str(), ".caldav", calendar_uid.c_str(), ".calendar"];
			NSString *path_exchange = [NSString stringWithFormat:@"%@%s%s/%s%s", userCalendarPath, group_uid.c_str(), ".exchange", calendar_uid.c_str(), ".calendar"];
			
			BOOL isDirectory;
			C_TEXT temp;
			
			if(([defaultManager fileExistsAtPath:path_caldav isDirectory:&isDirectory]) && isDirectory)
			{
				temp.setUTF16String([NSString stringWithFormat:@"%@/Events/", path_caldav]);
				temp.copyUTF8String(&path);
				success = YES;
			}
			else if(([defaultManager fileExistsAtPath:path_exchange isDirectory:&isDirectory]) && isDirectory)
			{
				temp.setUTF16String([NSString stringWithFormat:@"%@/Events/", path_exchange]);
				temp.copyUTF8String(&path);
				success = YES;
			}
			[defaultManager release];
		}
		
		[title release];
		
		if(success) break;
	}
	
	[titleToSearch release];
	
	return success;
}

void ON_APPOINTMENT_CALL(sLONG_PTR *pResult, PackagePtr pParams)
{
	C_TEXT Param1;
	C_TEXT Param2;
	
	Param1.fromParamAtIndex(pParams, 1);
	Param2.fromParamAtIndex(pParams, 2);
	
	method_id_t methodId = PA_GetMethodID((PA_Unichar *)Param2.getUTF16StringPtr());
	
	if(methodId)
	{
		CUTF8String path;
		if(json_get_calendarPath(Param1, path))
		{
			CalendarWatch::addToWatch(path, methodId);
		}
	}
//	APPOINTMENT_CTX._method.fromParamAtIndex(pParams, 1);
//	APPOINTMENT_CTX.method = APPOINTMENT_CTX._method.getUTF16StringPtr();
//	APPOINTMENT_CTX.method_size = APPOINTMENT_CTX._method.getUTF16Length();
//	APPOINTMENT_CTX.method_id = PA_GetMethodID((PA_Unichar *)APPOINTMENT_CTX.method);
}

void ALL_APPOINTMENTS(sLONG_PTR *pResult, PackagePtr pParams)
{
	C_TEXT Param1;
	C_TEXT Param2;
	
	Param1.fromParamAtIndex(pParams, 1);
	
	JSONNODE *ee = json_new(JSON_ARRAY);
	
	CalCalendarStore *defaultCalendarStore = [CalCalendarStore defaultCalendarStore];
	
	if(defaultCalendarStore)
	{
		CalCalendar *calendar = json_get_calendar(nil, Param1);
		if(calendar)
		{
			NSDate *startDate = [NSDate dateWithTimeIntervalSinceNow:-60 * 60 * 24 * 1000];
			NSDate *endDate = [NSDate dateWithTimeIntervalSinceNow:60 * 60 * 24 * 1000];
			
			NSPredicate *predicate = [CalCalendarStore eventPredicateWithStartDate:startDate
																																		 endDate:endDate
																																	 calendars:[NSArray arrayWithObject:calendar]];
			if(predicate)
			{
				NSArray *events = [[CalCalendarStore defaultCalendarStore]eventsWithPredicate:predicate];
				for(unsigned int j = 0; j < [events count]; ++j)
				{
					CalEvent *event = [events objectAtIndex:j];
					JSONNODE *e = json_new(JSON_NODE);
					json_set_event(e, event);
					json_push_back(ee, e);
				}
			}
		}
	}
	
	json_stringify(ee, Param2);
	json_delete(ee);
	
	Param2.toParamAtIndex(pParams, 2);
}

void APPOINTMENT_NAMES(sLONG_PTR *pResult, PackagePtr pParams)
{
	ARRAY_TEXT Param1;
	C_TEXT Param2;

	JSONNODE *ee = json_new(JSON_ARRAY);
	
	CalCalendarStore *defaultCalendarStore = [CalCalendarStore defaultCalendarStore];
	
	if(defaultCalendarStore)
	{
		Param1.setSize(1);
		NSArray *calendars = [defaultCalendarStore calendars];
		for(NSUInteger i = 0; i < [calendars count]; ++i)
		{
			CalCalendar *c = [calendars objectAtIndex:i];
			Param1.appendUTF16String([c title]);
		}
	}
	
	json_stringify(ee, Param2);
	json_delete(ee);
	
	Param1.toParamAtIndex(pParams, 1);
	Param2.toParamAtIndex(pParams, 2);
}

#pragma mark JSON Set

void json_set_text(JSONNODE *n, const wchar_t *name, NSString *value)
{
	if((n) && (name))
	{
		C_TEXT t;
		t.setUTF16String(value);
		
		uint32_t dataSize = (t.getUTF16Length() * sizeof(wchar_t))+ sizeof(wchar_t);
		std::vector<char> buf(dataSize);
		
		PA_ConvertCharsetToCharset((char *)t.getUTF16StringPtr(),
															 t.getUTF16Length() * sizeof(PA_Unichar),
															 eVTC_UTF_16,
															 (char *)&buf[0],
															 dataSize,
															 eVTC_UTF_32);
		
		json_push_back(n, json_new_a(name, (wchar_t *)&buf[0]));
	}
}

void json_set_date(JSONNODE *n, const wchar_t *name, NSDate *value, BOOL isGMT = YES)
{
	if(isGMT)
	{
		json_set_text(n, name, value ? [DateFormatter::GMT stringFromDate:value] : @"");
	}else
	{
		json_set_text(n, name, value ? [DateFormatter::ISO stringFromDate:value] : @"");
	}
}

void json_set_url(JSONNODE *n, const wchar_t *name, NSURL *url)
{
	json_set_text(n, name, url ? [url absoluteString] : @"");
}

void json_set_time(JSONNODE *n, const wchar_t *name, NSDate *date)
{
	if((n) && (name) && (date))
	{
		NSString *localDateString = [DateFormatter::ISO stringFromDate:date];
		
		int hour = [[localDateString substringWithRange:NSMakeRange(11,2)]integerValue];
		int minute = [[localDateString substringWithRange:NSMakeRange(14,2)]integerValue];
		int second = [[localDateString substringWithRange:NSMakeRange(17,2)]integerValue];
		int time = (second + (minute * 60) + (hour * 3600)) * 1000;
		
		json_push_back(n, json_new_i(name, time));
	}
}

void json_set_event(JSONNODE *obj, CalEvent *event)
{
	json_set_text(obj, L"location", [event location]);
	json_set_text(obj, L"notes", [event notes]);
	json_set_text(obj, L"title", [event title]);
	json_set_url(obj, L"url", [event url]);
	json_set_text(obj, L"uid", [event uid]);
	json_set_date(obj, L"endDate", [event endDate]);
	json_set_date(obj, L"startDate", [event startDate]);
	json_set_date(obj, L"dateStamp", [event dateStamp]);
	json_set_time(obj, L"endDateTime", [event endDate]);
	json_set_time(obj, L"startDateTime", [event startDate]);
	json_set_time(obj, L"dateStampTime", [event dateStamp]);
}

#pragma mark JSON Get

BOOL json_get_string(JSONNODE *json, const wchar_t *name, CUTF8String &value)
{
	value = (const uint8_t *)"";
	
	if(json)
	{
		JSONNODE *node = json_get(json, name);
		if(node)
		{
			json_char *s =json_as_string(node);
			
			std::wstring wstr = std::wstring(s);
			
			C_TEXT t;
			
#if VERSIONWIN
			t.setUTF16String((const PA_Unichar *)wstr.c_str(), (uint32_t)wstr.length());
#else
			uint32_t dataSize = (uint32_t)((wstr.length() * sizeof(wchar_t)) + sizeof(PA_Unichar));
			std::vector<char> buf(dataSize);
			
			uint32_t len = PA_ConvertCharsetToCharset((char *)wstr.c_str(),
																								(PA_long32)(wstr.length() * sizeof(wchar_t)),
																								eVTC_UTF_32,
																								(char *)&buf[0],
																								dataSize,
																								eVTC_UTF_16);
			
			t.setUTF16String((const PA_Unichar *)&buf[0], len);
#endif
			
			t.copyUTF8String(&value);
			
			json_free(s);
		}
	}
	
	return !!value.length();
}

NSString *json_get_text(JSONNODE *obj, const wchar_t *name)
{
	NSString *string = nil;
	
	CUTF8String value;
	if(json_get_string(obj, name, value))
	{
		string = [NSString stringWithUTF8String:(const char *)value.c_str()];
	}else
	{
		string = [NSString stringWithUTF8String:(const char *)""];
	}
	
	return string;
}

NSURL *json_get_url(JSONNODE *obj, const wchar_t *name)
{
	return [NSURL URLWithString:json_get_text(obj, name)];
}

NSDate *json_get_date(JSONNODE *obj, const wchar_t *name)
{
	NSDate *date = nil;
	
	CUTF8String value;
	if(json_get_string(obj, name, value))
	{
		C_TEXT temp;
		temp.setUTF8String(&value);
		NSString *s = temp.copyUTF16String();
		
		if([s hasSuffix:@"Z"])
		{
			date = [DateFormatter::GMT dateFromString:s];
		}else
		{
			date = [DateFormatter::ISO dateFromString:s];
		}
		
		[s release];
	}

	return date;
}

CalCalendarStore *json_get_calendar_store(JSONNODE *obj)
{
	CalCalendarStore *defaultCalendarStore = [CalCalendarStore defaultCalendarStore];
	
	if(obj)
	{
		json_set_text(obj, L"json_get_calendar_store", defaultCalendarStore ? @"OK" : @"calendar access denied");
	}
	return defaultCalendarStore;
}

CalEvent *json_get_event(JSONNODE *obj, C_TEXT& Param1)
{
	CalEvent *event = nil;
	CalCalendarStore *defaultCalendarStore = json_get_calendar_store(obj);
	
	if(defaultCalendarStore)
	{
		NSString *uid = Param1.copyUTF16String();
		event = [defaultCalendarStore eventWithUID:uid occurrence:nil];
		[uid release];
	}
	
	if(obj)
	{
		json_set_text(obj, L"json_get_event", event ? @"OK" : @"event not found");
	}
	
	return event;
}

CalCalendar *json_get_calendar(JSONNODE *obj, C_TEXT& Param1)
{
	CalCalendar *calendar = nil;
	CalCalendarStore *defaultCalendarStore = json_get_calendar_store(obj);
	
	if(defaultCalendarStore)
	{
		NSString *calendarName = Param1.copyUTF16String();
		NSArray *calendars = [defaultCalendarStore calendars];
		for(NSUInteger i = 0; i < [calendars count]; ++i)
		{
			CalCalendar *c = [calendars objectAtIndex:i];
			
			if(([[c title]caseInsensitiveCompare:calendarName] == NSOrderedSame)
				 && [[c type]isEqualToString:CalCalendarTypeCalDAV])
			{
				calendar = c;
				break;
			}
		}
	}
	
	if(obj)
	{
		json_set_text(obj, L"json_get_calendar", calendar ? @"OK" : @"calendar not found");
	}
	
	return calendar;
}


#pragma mark JSON

JSONNODE *json_parse(C_TEXT &t)
{
	std::wstring u32;
	
#if VERSIONWIN
	u32 = std::wstring((wchar_t *)t.getUTF16StringPtr());
#else
	
	uint32_t dataSize = (t.getUTF16Length() * sizeof(wchar_t))+ sizeof(wchar_t);
	std::vector<char> buf(dataSize);
	
	PA_ConvertCharsetToCharset((char *)t.getUTF16StringPtr(),
														 t.getUTF16Length() * sizeof(PA_Unichar),
														 eVTC_UTF_16,
														 (char *)&buf[0],
														 dataSize,
														 eVTC_UTF_32);
	
	u32 = std::wstring((wchar_t *)&buf[0]);
#endif
	return json_parse((json_const json_char *)u32.c_str());
}

void json_stringify(JSONNODE *json, C_TEXT &t)
{
	json_char *json_string = json_write_formatted(json);
	std::wstring wstr = std::wstring(json_string);
	uint32_t dataSize = (uint32_t)((wstr.length() * sizeof(wchar_t))+ sizeof(PA_Unichar));
	std::vector<char> buf(dataSize);
	uint32_t len = PA_ConvertCharsetToCharset((char *)wstr.c_str(),
																						(PA_long32)(wstr.length() * sizeof(wchar_t)),
																						eVTC_UTF_32,
																						(char *)&buf[0],
																						dataSize,
																						eVTC_UTF_16);
	t.setUTF16String((const PA_Unichar *)&buf[0], len);
	json_free(json_string);
}

#pragma mark -

void event_to_json(CUTF16String *eventId, CUTF16String *eventJson)
{
	JSONNODE *r = json_new(JSON_NODE);
	if(r)
	{
		C_TEXT Param1;
		Param1.setUTF16String(eventId);
		
		CalEvent *event = json_get_event(r, Param1);
		
		if(event)
		{
			JSONNODE *ee = json_new(JSON_ARRAY);
			JSONNODE *e = json_new(JSON_NODE);
			
			json_set_event(e, event);
			
			json_push_back(ee, e);
			
			json_stringify(ee, Param1);
			
			Param1.copyUTF16String(eventJson);
			json_delete(ee);
		}
		json_delete(r);
	}
}
