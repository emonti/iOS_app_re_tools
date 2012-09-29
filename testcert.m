#import <Foundation/Foundation.h>
#import <Security/Security.h>
#import <CoreFoundation/CoreFoundation.h>

bool isCertValid(CFStringRef leafName, SecCertificateRef leafCert) {
  bool ret=false;
  SecTrustRef trust;
  SecTrustResultType res;
  
  // Create the policy so that the leaf cert is verified for leafName.
  // If leafName is NULL the function will still work but the workaround
  // doesn't work.
  SecPolicyRef policy = SecPolicyCreateSSL(true, leafName);
  OSStatus status = SecTrustCreateWithCertificates((void *)leafCert, policy, &trust);
  
  if ((status == noErr) &&
      (SecTrustEvaluate(trust, &res) == errSecSuccess) && 
      ((res == kSecTrustResultProceed) || (res == kSecTrustResultUnspecified))) 
  { ret = true; }
  
  if (trust) CFRelease(trust);
  if (policy) CFRelease(policy); 
  
  return ret;
}


int main(int argc, char **argv)
{
  int i;

  if (argc < 3) {
    fprintf(stderr, "usage: %s path/to/file.der severname\n", argv[0]);
    exit(1);
  }

  NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];

  NSString *thePath=[NSString stringWithUTF8String:argv[1]];
  CFStringRef serverName = CFStringCreateWithCString(NULL, argv[2], 0);

  CFDataRef certData = (CFDataRef)[[NSData alloc] initWithContentsOfFile:thePath];
  SecCertificateRef cert = SecCertificateCreateWithData(kCFAllocatorDefault, certData);

  CFStringRef desc = SecCertificateCopySubjectSummary(cert);
  if (desc != NULL) {
    printf("Certificate Description: %s\n", CFStringGetCStringPtr(desc, 0));
    CFRelease(desc);
  }
 
  printf("Certificate is ");
  if(isCertValid(serverName, cert))
    printf("VALID\n");
  else
    printf("INVALID!!!\n");

  [pool release];
  return 0;
}
