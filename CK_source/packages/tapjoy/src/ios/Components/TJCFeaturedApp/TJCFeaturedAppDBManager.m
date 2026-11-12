#import "TJCFeaturedAppDBManager.h"
#import <sqlite3.h>

@implementation TJCFeaturedAppDBManager
+ (TJCFeaturedAppDBManager*)sharedTJCFeaturedAppDBManager
{
    static TJCFeaturedAppDBManager* instance;
    if (instance == nil) {
        instance = [[self alloc] init];
    }
    return instance;
}

- (id)init
{
    self = [super init];
    if (self) {
        NSArray* directories = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
        if ([directories count] != 0) {
            NSString* directory = [directories objectAtIndex:0];
            currentDBPath_ = [[directory stringByAppendingPathComponent:@"featured_app.sql"] copy];
        }
    }
    return self;
}

- (int)execute:(const char*)sql storeID:(NSString*)storeID
{
    if ([storeID length] == 0 || currentDBPath_ == nil) {
        return 0;
    }
    sqlite3* database = NULL;
    int result = 0;
    if (sqlite3_open([currentDBPath_ fileSystemRepresentation], &database) == SQLITE_OK) {
        const char* create = "CREATE TABLE IF NOT EXISTS featured_apps "
                             "(store_id TEXT PRIMARY KEY, no_of_times_displayed INTEGER NOT NULL)";
        if (sqlite3_exec(database, create, NULL, NULL, NULL) == SQLITE_OK) {
            sqlite3_stmt* statement = NULL;
            if (sqlite3_prepare_v2(database, sql, -1, &statement, NULL) == SQLITE_OK) {
                sqlite3_bind_text(statement, 1, [storeID UTF8String], -1, SQLITE_TRANSIENT);
                if (sqlite3_step(statement) == SQLITE_ROW) {
                    result = sqlite3_column_int(statement, 0);
                }
            }
            sqlite3_finalize(statement);
        }
    }
    sqlite3_close(database);
    return result;
}

- (void)addApp:(TJCFeaturedAppModel*)app
{
    [self execute:"INSERT OR IGNORE INTO featured_apps (store_id, no_of_times_displayed) VALUES (?1, 1)"
          storeID:[app storeID]];
}

- (void)incrementDisplayedCountForStoreID:(NSString*)storeID
{
    [self execute:"UPDATE featured_apps SET no_of_times_displayed = no_of_times_displayed + 1 WHERE store_id = ?1"
          storeID:storeID];
}

- (int)getDisplayedCountForStoreID:(NSString*)storeID
{
    return [self execute:"SELECT no_of_times_displayed FROM featured_apps WHERE store_id = ?1" storeID:storeID];
}

- (void)dealloc
{
    [currentDBPath_ release];
    [super dealloc];
}
@end
