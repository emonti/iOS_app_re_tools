#import <Foundation/Foundation.h>
#import <Security/Security.h>
#import <sqlite3.h>
#import <unistd.h>

void dump_keychain(char *name, id secClass)
{
    NSArray *kc=nil;
    NSMutableDictionary *dct=[[[NSMutableDictionary alloc] init] autorelease];

    [dct setObject:(id)secClass forKey:(id)kSecClass];
    [dct setObject:(id)kSecMatchLimitAll forKey:(id)kSecMatchLimit];
    [dct setObject:(id)kCFBooleanTrue forKey:(id)kSecReturnAttributes];
    [dct setObject:(id)kCFBooleanTrue forKey:(id)kSecReturnData];
    
    if (SecItemCopyMatching((CFDictionaryRef)dct, (CFTypeRef *)&kc) == noErr)
        printf("\n[*] %s Keychain:\n%s\n\n", name, [[kc description] UTF8String]);
    else
        fprintf(stderr, "Error: %s items could not be dumped\n\n", name);
} 


void dump_result_as_plist(sqlite3_stmt *stmt) {
    printf("<plist version=\"1.0\"><dict><key>keychain-access-groups</key><array>");
    while(sqlite3_step(stmt) == SQLITE_ROW)
        printf("\n  <string>%s</string>", sqlite3_column_text(stmt, 0));
    printf("\n</array></dict></plist>\n");
}


int dump_entitlements_from_db()
{
    int ret;
    sqlite3 *kcdb;
    sqlite3_stmt *stmt;
    
    if (sqlite3_open("/var/Keychains/keychain-2.db", &kcdb) == SQLITE_OK) {
        const char *query_stmt="SELECT DISTINCT agrp FROM genp "
                               "UNION SELECT DISTINCT agrp FROM inet "
                               "UNION SELECT DISTINCT agrp FROM cert "
                               "UNION SELECT DISTINCT agrp FROM keys";
        
        if (sqlite3_prepare_v2(kcdb, query_stmt, -1, &stmt, NULL) == SQLITE_OK) {
            dump_result_as_plist(stmt);
            sqlite3_finalize(stmt);
            ret = 0;
        } else {
            fprintf(stderr,"Keychain DB query error!\n");
            ret = 1;
        }
        sqlite3_close(kcdb);
    } else {
        fprintf(stderr,"Keychain DB access error!\n");
        ret = 1;
    }
    return(ret);
}


void usage(char *pname) {
    fprintf(stderr, "Usage: %s [hpagIick]\n", pname);
    fprintf(stderr, " options:\n"
                    "   -?,-h  Show this help message\n"
                    "      -p  Dump entitlements plist for ldid and exit\n"
                    "      -a  Dump all keychains (default)\n"
                    "      -g  Dump General Passwords\n"
                    "      -I  Dump Internet Passwords\n"
                    "      -i  Dump Identities\n"
                    "      -c  Dump Certificates\n"
                    "      -k  Dump Keys\n");
}

int main(int argc, char **argv) 
{
    bool all=false, genp=false, inet=false, iden=false, cert=false, keys=false;
    id pool=[NSAutoreleasePool new];
    int ch;

    while ((ch = getopt(argc, argv, "?hpagiIck")) != -1) {
        switch(ch) {
            case 'p':
                exit(dump_entitlements_from_db());
            case 'a': all=true; break;
            case 'g': genp=true; break;
            case 'I': inet=true; break;
            case 'i': iden=true; break;
            case 'c': cert=true; break;
            case 'k': keys=true; break;
            case '?': 
            case 'h': 
             default:
                usage(argv[0]);
                exit(1);
        }
    }

    if (argc == 1 || all)
        genp=inet=iden=cert=keys=true;

    argc -= optind;
    argv += optind;

    if (genp)
        dump_keychain("Generic Password", (id)kSecClassGenericPassword);

    if (inet)
        dump_keychain("Internet Password", (id)kSecClassInternetPassword);

    if (iden)
        dump_keychain("Identity", (id)kSecClassIdentity);

    if (keys)
        dump_keychain("Key", (id)kSecClassKey);

    if (cert)
        dump_keychain("Certificate", (id)kSecClassCertificate);

    [pool drain];
    exit(0);
}

