#import <UIKit/UIKit.h>
#import <Security/Security.h>
#import "sqlite3.h"

void dump(NSString *format, ...) 
{
    va_list args;
    va_start(args, format);
    NSString *formattedString = [[NSString alloc] initWithFormat:format arguments:args];
    va_end(args);
    printf("%s", [formattedString UTF8String]);
    [formattedString release];
}


void dump_keychain(char *label, id secClass)
{
    NSArray *keychain = nil;
    NSMutableDictionary *query = [[NSMutableDictionary alloc] init];

    [query setObject:(id)secClass forKey:(id)kSecClass];
    [query setObject:(id)kSecMatchLimitAll forKey:(id)kSecMatchLimit];
    [query setObject:(id)kCFBooleanTrue forKey:(id)kSecReturnAttributes];
    [query setObject:(id)kCFBooleanTrue forKey:(id)kSecReturnData];
    
    if (SecItemCopyMatching((CFDictionaryRef)query, (CFTypeRef *)&keychain) == noErr)
        dump(@"#### Dumping %s Keychain:\n%@\n\n", label, [keychain description]);
    else
        fprintf(stderr, "Error: %s items could not be dumped\n\n", label);
    
    [keychain release];
} 


void dump_result_as_plist(sqlite3_stmt *stmt) {
    printf("<plist version=\"1.0\"><dict><key>keychain-access-groups</key><array>");
    while(sqlite3_step(stmt) == SQLITE_ROW)
        printf("\n  <string>%s</string>", sqlite3_column_text(stmt, 0));
    printf("\n</array></dict></plist>\n");
}


void dump_entitlements_from_db()
{
    sqlite3 *kcdb;
    sqlite3_stmt *stmt;
    
    if (sqlite3_open("/var/Keychains/keychain-2.db", &kcdb) == SQLITE_OK) {
        const char *query_stmt = "SELECT DISTINCT agrp FROM genp UNION SELECT DISTINCT agrp FROM inet UNION SELECT DISTINCT agrp FROM cert UNION SELECT DISTINCT agrp FROM keys";
        
        if (sqlite3_prepare_v2(kcdb, query_stmt, -1, &stmt, NULL) == SQLITE_OK) {
            dump_result_as_plist(stmt);
            sqlite3_finalize(stmt);
        } else {
            fprintf(stderr,"Keychain DB query error!\n");
        }
        sqlite3_close(kcdb);
    } else {
        fprintf(stderr,"Keychain DB access error!\n");
    }
}


int main(int argc, char **argv) 
{
    int dumpent = 1;
    id pool=[NSAutoreleasePool new];

    if (argc > 2 || (argc == 2 && (dumpent=strncmp(argv[1], "-p", 2) != 0))) {
        fprintf(stderr, "Usage: %s [-p]\n", argv[0]);
        fprintf(stderr, "  -p dumps entitlements plist for use with ldid\n");
        exit(1);
    } else if (dumpent == 0) {
        dump_entitlements_from_db();
        exit(0);
    } else {
        dump_keychain("Generic Password", (id)kSecClassGenericPassword);
        dump_keychain("Internet Password", (id)kSecClassInternetPassword);
        dump_keychain("Identity", (id)kSecClassIdentity);
        dump_keychain("Key", (id)kSecClassKey);
        dump_keychain("Certificate", (id)kSecClassCertificate);
    }

    [pool drain];
    exit(0);
}

