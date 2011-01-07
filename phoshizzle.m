/* phoshizzle.m by eric monti
 *
 * Hook [NSObject init] and watch children get created at runtime in the logs.
 *
 * compile with -dynamiclib -init _hook_setup
 */

#import <Foundation/Foundation.h>
#import <objc/runtime.h>

// This macro sets up a hook into the objective-C runtime
#define HookObjC(cl, sel, new, bak) \
 (*(bak) = method_setImplementation(class_getInstanceMethod((cl), (sel)), (new)))

// Holds a pointer to the original [NSObject init]
static IMP hook_orig;

// our overridden [NSObject init] hook
id hook_init(id _self, SEL _cmd) {
  NSLog(@"Class Initialized: %@", [_self class]);
  return hook_orig(_self, _cmd);
}

void hook_setup(void)
{
  HookObjC(objc_getClass("NSObject"),
           @selector(init),
           (IMP) hook_init,
           (IMP *) &hook_orig);
}  
