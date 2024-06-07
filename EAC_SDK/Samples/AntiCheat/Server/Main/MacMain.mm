// Copyright Epic Games, Inc. All Rights Reserved.

#include "MacMain.h"

#if __APPLE__

#import <CoreFoundation/CoreFoundation.h>
#import <AppKit/AppKit.h>

@interface AppDelegate : NSObject <NSApplicationDelegate>
@property (nonatomic, assign) int argc;
@property (nonatomic, assign) const char** argv;
@property (atomic, retain) NSThread* logicThread;
@property (nonatomic, assign) int exitCode;
@property (nonatomic, assign) int (*Driver)(int, const char* []);

- (id)initWithCommandLineParams:(int)argc argv:(const char* [])argv driver:(int (*)(int, const char* []))Driver;
@end

@implementation AppDelegate
- (id)initWithCommandLineParams:(int)argc argv:(const char* [])argv driver:(int (*)(int, const char* []))Driver
{
	if (!Driver) {
		return nil;
	}
	
	if (self = [super init]) {
		_argc = argc;
		_argv = argv;
		_Driver = Driver;
		_logicThread = NULL;
		_exitCode = -1;
	}
	return self;
}

- (void)dealloc
{
	[_logicThread release];
	[super dealloc];
}

- (void)handleQuitEvent:(NSAppleEventDescriptor*)event withReplyEvent:(NSAppleEventDescriptor*)replyEvent
{
	self.exitCode = -1;
	[self terminate];
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender
{
	self.exitCode = -1;
	[self terminate];
	return NSTerminateCancel;
}

- (void)applicationDidFinishLaunching:(NSNotification *)notification
{
	NSAppleEventManager* eventManager = [NSAppleEventManager sharedAppleEventManager];
	[eventManager setEventHandler:self andSelector:@selector(handleQuitEvent:withReplyEvent:) forEventClass:kCoreEventClass andEventID:kAEQuitApplication];
	
	self.logicThread = [[NSThread alloc] initWithBlock:^{
		self.exitCode = (*self.Driver)(self.argc, self.argv);
		[self terminate];
	}];
	[self.logicThread start];
}

- (void)terminate
{
	NSEvent* dummyEvent = [NSEvent otherEventWithType:NSEventTypeApplicationDefined location:{0, 0} modifierFlags:0 timestamp:0 windowNumber:0 context:nil subtype:0 data1:0 data2:0];
	[NSApp stop:self];
	[NSApp postEvent:dummyEvent atStart:TRUE];
}

@end

void DisableAppNap(void)
{
	if ([[NSProcessInfo processInfo] respondsToSelector:@selector(beginActivityWithOptions:reason:)])
	{
		[[NSProcessInfo processInfo] beginActivityWithOptions:0x00FFFFFF reason:@"Not sleepy and don't want to nap"];
	}
}

#endif

int MainDriverMac(int argc, const char* argv[], int (*Driver)(int, const char*[]))
{
#if __APPLE__
	DisableAppNap();
	NSAutoreleasePool* AutoreleasePool = [NSAutoreleasePool new];
	NSApplication* app = [NSApplication sharedApplication];
	AppDelegate* delegate = [[AppDelegate alloc] initWithCommandLineParams:argc argv:argv driver:Driver];
	app.delegate = delegate;
	[app run];
	app.delegate = NULL;
	
	int returnValue = [delegate exitCode];
	[delegate release];
	[AutoreleasePool release];
	return returnValue;
#else
	(void)Driver;
	return 0;
#endif
}

