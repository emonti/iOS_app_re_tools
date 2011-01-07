/*
 * phoshizzle2.dylib by eric monti
 *
 * A basic hook PoC designed to make Just Light turn the screen red instead 
 * of white.
 *
 * compile with -dynamiclib -init _hook_setup
 */

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import <objc/runtime.h>

// This macro sets up a hook into the objective-C runtime
#define HookObjC(cl, sel, new, bak) \
 (*(bak) = method_setImplementation(class_getInstanceMethod((cl), (sel)), (new)))

static IMP hook_orig;

void hook(id _self, SEL _cmd, id arg) {
  [_self setBackgroundColor:[UIColor redColor]];
  hook_orig(_self, _cmd, arg);
}

void hook_setup(void)
{
  id cl = objc_getClass("UIWindow");
  HookObjC(cl, @selector(makeKeyAndVisible), 
           (IMP) hook, (IMP *) &hook_orig);
}
