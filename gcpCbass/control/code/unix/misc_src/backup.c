#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/param.h>  /* MAXHOSTNAMELEN */
#include <pwd.h>
#include <netdb.h>      /* MAXHOSTNAMELEN */
#include <errno.h>

#include "hash.h"
#include "input.h"
#include "pathname.h"

/*-----------------------------------------------------------------------
 * This program is a suid-root wrapper around the Solaris ufsdump()
 * program. It needs to be installed set-uid-root (man chmod) so that
 * ufsdump can access raw filesystems.  A system-wide configuration file,
 * described below is used to authorize trusted users to use the program
 * and to explicitly specify which filesystems can be backed up to which
 * backup devices. Note that only the forked process which runs ufsdump
 * is actually run as root. The rest of the program is run as the user
 * who invoked the backup program.
 *
 * An example configuration file is shown below between the lines:
 *
 *-----------------------------------------------------------------------
 *   # Specify the list of users who are allowed to make backups, or
 *   # specify * to say that all users are authorized.
 *
 *   users mcs tjp sp
 *
 *   # Specify the names by which ufsdump levels are to be specified.
 *
 *   level all 0     # Backup all files on the specified filesystems.
 *   level new 1     # Backup any files that have been added or changed
 *                   #  since the last backup in which level 0 (ie. all)
 *                   #  was specified.
 *
 *   # Give names to the local filesystems that can be backed up by the
 *   # backup program.
 *
 *   fs home /export/home
 *   fs scratch /export/scr
 *   fs root /
 *
 *   # Give names to the devices onto which backups can be made, specifying
 *   # the devices themselves in the form documented for ufsdump.
 *
 *   device tape /dev/rmt/0n
 *   device tape@lyra lyra:/dev/rmt/0n
 *   device optical /optical1
 *-----------------------------------------------------------------------
 *
 * Note that at run time, if the chosen backup device is found to be
 * a directory (as opposed to a tape device), then the dump will
 * be written as a file within that directory. For example, if I were
 * to run:
 *
 *   backup new optical home root
 *
 * On a computer called vega, then two files would be created under
 * /optical1, called vega_home_dump.1 and vega_root_dump.1. The
 * postfixed number is the dump level.
 */

/*
 * Specify the name and location of the configuration file.
 */
#define CONF_FILE_NAME "/usr/local/etc/backup.conf"

/*
 * Set the blocking factors for allocating HashTable and HashNode
 * objects.
 */
#define HASH_TABLE_COUNT 4
#define HASH_NODE_COUNT 20

/*
 * Set the sizes of the hash-tables. This should be a prime number.
 */
#define HASH_TABLE_SIZE 13

/*
 * An object of the following type is used to record the contents of
 * the above configuration file.
 */
typedef struct {
  HashMemory *mem;    /* The memory for hash tables */
  int everybody;      /* If true, all users are allowed, and the 'users' */
                      /*  hash-table can be ignored. */
  HashTable *users;   /* The table of users that are allowed to run this */
                      /*  program. */
  HashTable *levels;  /* The symbol table of aliases for ufsdump levels */
  HashTable *fs;      /* The symbol table of aliases for file-systems that */
                      /*  can be backed up. */
  HashTable *devices; /* The symbol table of aliases for backup devices */
} ConfigFile;

static ConfigFile *new_ConfigFile(char *file);
static int input_backup_user_list(InputStream *stream, ConfigFile *cf);
static int input_backup_level_alias(InputStream *stream, ConfigFile *cf);
static int input_file_system_alias(InputStream *stream, ConfigFile *cf);
static int input_backup_device_alias(InputStream *stream, ConfigFile *cf);
static ConfigFile *bad_ConfigFile(ConfigFile *cf, InputStream *stream);
static ConfigFile *del_ConfigFile(ConfigFile *cf);
static SYM_DEL_FN(del_hash_obj);
static int user_is_authorized(char *prog, ConfigFile *cf);
static int lookup_dump_level(char *prog, ConfigFile *cf, char *name);
static char *lookup_backup_device(char *prog, ConfigFile *cf, char *name);
static char *lookup_filesystem(char *prog, ConfigFile *cf, char *name);
static int backup_file_system(char *prog, int level, char *device, char *fs,
			      int to_disk, char *fs_alias, char *host);
static void show_usage(char *prog);

/*
 * The signal_catcher() signal handler sets caught_sigchld to 1 when
 * it receives a SIGCHLD signal.
 */
static volatile sig_atomic_t caught_sigchld = 0;
static void signal_catcher(int sig);

int main(int argc, char *argv[])
{
  char *prog;        /* The name of the program */
  ConfigFile *cf;    /* The configuration file */
  int level;         /* The dump level */
  int to_disk;       /* True if the backup device is a normal directory */
  char *device;      /* The backup-device specification */
  char host[MAXHOSTNAMELEN]; /* The name of the parent computer */
  int i;
/*
 * There must be at least 3 arguments in addition to the command name.
 */
  if(argc < 4) {
    show_usage((char *)(argc < 1 ? "backup":argv[0]));
    return 1;
  };
/*
 * Get the name of the program.
 */
  prog = argv[0];
/*
 * Read the configuration file.
 */
  cf = new_ConfigFile(CONF_FILE_NAME);
  if(!cf)
    return 1;
/*
 * Check whether the user is authorized to use this program.
 */
  if(!user_is_authorized(prog, cf)) {
    cf = del_ConfigFile(cf);
    return 1;
  };
/*
 * Get the ufsdump level.
 */
  level = lookup_dump_level(prog, cf, argv[1]);
  if(level < 0) {
    cf = del_ConfigFile(cf);
    return 1;
  };
/*
 * Get the backup device.
 */
  device = lookup_backup_device(prog, cf, argv[2]);
  if(!device) {
    cf = del_ConfigFile(cf);
    return 1;
  };
/*
 * Determine whether the backup device is a tape drive, or a filesystem.
 */
  to_disk = device[0]=='/' && test_pathname(device, PATH_IS_DIR, PATH_OK)==NULL;
/*
 * Get the name of the current host computer.
 */
  if(gethostname(host, sizeof(host)) == -1) {
    fprintf(stderr, "gethostname: %s\n", strerror(errno));
    cf = del_ConfigFile(cf);
    return 1;
  };
/*
 * Precheck the filesystem names.
 */
  for(i=3; i<argc; i++) {
    if(!lookup_filesystem(prog, cf, argv[i])) {
      cf = del_ConfigFile(cf);
      return 1;
    };
  };
/*
 * Now backup each of the file-systems in turn.
 */
  for(i=3; i<argc; i++) {
    char *fs = lookup_filesystem(prog, cf, argv[i]);
    if(!fs || backup_file_system(prog, level, device, fs, to_disk, argv[i],
				 host)) {
      cf = del_ConfigFile(cf);
      return 1;
    };
  };
/*
 * Delete the configuration database.
 */
  cf = del_ConfigFile(cf);
  return 0;
}

/*.......................................................................
 * Indicate the correct way to call the backup program.
 *
 * Input:
 *  prog     char *  The name of the program.
 */
static void show_usage(char *prog)
{
  fprintf(stderr, "Usage: %s <level> <where> <filesystem>...\n", prog);
}

/*.......................................................................
 * Read the configuration file.
 *
 * This file should contain lines of the form:
 *
 *  users *             - Allow all users to run the backup program.
 *  users user...       - Allow the named users to run the backup program.
 *  level name number   - Give a name to one of the ufsdump dump levels.
 *                        The level number must be in the range 0..9.
 *  fs name system      - Give a name to a file-system that can be
 *                        backed up by the backup program.
 *  device name device  - Name a backup device. The actual device
 *                        specification must be given in a form that
 *                        is acceptable to ufsdump.
 * Input:
 *  file          char *  The name of the configuration file.
 * Output:
 *  return  ConfigFile *  The contents of the configuration file.
 */
static ConfigFile *new_ConfigFile(char *file)
{
  InputStream *stream = NULL;   /* The configuration file parser */
  ConfigFile *cf = NULL;        /* The contents of the configuration file */
/*
 * Allocate the container of the ConfigFile object.
 */
  cf = (ConfigFile* )malloc(sizeof(ConfigFile));
  if(!cf) {
    fprintf(stderr, "new_ConfigFile(%s): Insufficient memory.\n", file);
    return NULL;
  };
/*
 * Before attempting any operation that might fail, initialize the
 * container at least up to the point at which it can be passed to
 * del_ConfigFile().
 */
  cf->mem = NULL;
  cf->everybody = 0;
  cf->users = NULL;
  cf->levels = NULL;
  cf->fs = NULL;
  cf->devices = NULL;
/*
 * Create the free-lists that are used to allocate hash-tables and
 * hash-nodes.
 */
  cf->mem = new_HashMemory(HASH_TABLE_COUNT, HASH_NODE_COUNT);
  if(!cf->mem)
    return bad_ConfigFile(cf, stream);
/*
 * Create a hash-table for recording valid users.
 */
  cf->users = new_HashTable(cf->mem, HASH_TABLE_SIZE, IGNORE_CASE, NULL, 0);
  if(!cf->users)
    return bad_ConfigFile(cf, stream);
/*
 * Create a hash-table for recording and naming accepted ufsdump levels.
 */
  cf->levels = new_HashTable(cf->mem, HASH_TABLE_SIZE, IGNORE_CASE, NULL, 0);
  if(!cf->levels)
    return bad_ConfigFile(cf, stream);
/*
 * Create a hash-table for recording and naming accepted file-systems.
 */
  cf->fs = new_HashTable(cf->mem, HASH_TABLE_SIZE, IGNORE_CASE, NULL, 0);
  if(!cf->fs)
    return bad_ConfigFile(cf, stream);
/*
 * Create a hash-table for recording and naming accepted backup devices.
 */
  cf->devices = new_HashTable(cf->mem, HASH_TABLE_SIZE, IGNORE_CASE, NULL, 0);
  if(!cf->devices)
    return bad_ConfigFile(cf, stream);
/*
 * Create a parser input stream.
 */
  stream = new_InputStream();
  if(!stream)
    return NULL;
/*
 * Open the configuration file.
 */
  if(open_FileInputStream(stream, "", file))
    return NULL;
/*
 * Parse the file.
 */
  while(input_skip_white(stream, 1, 0) || stream->nextc != EOF) {
/*
 * Get the first word on the line. This specifies the type of
 * line that is to be read.
 */
    if(input_keyword(stream, 0, 1) ||
       input_skip_space(stream, 0, 0))
      return bad_ConfigFile(cf, stream);
/*
 * Find out what type of line to expect.
 */
    if(strcmp(stream->work, "users") == 0) {
      if(input_backup_user_list(stream, cf))
	return bad_ConfigFile(cf, stream);
    } else if(strcmp(stream->work, "level") == 0) {
      if(input_backup_level_alias(stream, cf))
	return bad_ConfigFile(cf, stream);
    } else if(strcmp(stream->work, "fs") == 0) {
      if(input_file_system_alias(stream, cf))
	return bad_ConfigFile(cf, stream);
    } else if(strcmp(stream->work, "device") == 0) {
      if(input_backup_device_alias(stream, cf))
	return bad_ConfigFile(cf, stream);
    } else {
      input_error(stream, 1, "Unknown line-type specifier: (%s)\n",
		  stream->work);
      return bad_ConfigFile(cf, stream);
    };
  };
/*
 * Close the configuration file and discard the parser stream.
 */
  stream = del_InputStream(stream);
  return cf;
}

/*.......................................................................
 * Read the list of users that are allowed to run the backup program.
 * The special username '*' is a wildcard for everybody.
 *
 * Input:
 *  stream     InputStream *   The stream to read the list from.
 *  cf          ConfigFile *   The place to record the results.
 * Output:
 *  return             int     0 - OK.
 *                             1 - Error.
 */
static int input_backup_user_list(InputStream *stream, ConfigFile *cf)
{
  while(input_skip_space(stream, 0, 0)==0 && stream->nextc != '\n') {
/*
 * A username given as '*' is treated as a wildcard for all users.
 */
    if(stream->nextc == '*') {
      if(input_skip_space(stream, 0, 1) || !isspace(stream->nextc)) {
	input_error(stream, 1, "Bad username.\n");
	return 1;
      };
      cf->everybody = 1;
/*
 * A normal username.
 */
    } else {
/*
 * Read the next username.
 */
      if(input_literal(stream, 0, "", "", isgraph, "")) {
	input_error(stream, 1, "Bad username.\n");
	return 1;
      };
/*
 * Add the username to the table of valid users.
 */
      if(new_HashSymbol(cf->users, stream->work, 0, 0, NULL, 0) == NULL)
	return 1;
    };
  };
  return 0;
}

/*.......................................................................
 * Read the alias name by which the user can refer to a given ufsdump
 * level. The format of the record is the alias-name, followed by a
 * dump level number in the range 0..9.
 *
 * Input:
 *  stream     InputStream *   The stream to read the list from.
 *  cf          ConfigFile *   The place to record the results.
 * Output:
 *  return             int     0 - OK.
 *                             1 - Error.
 */
static int input_backup_level_alias(InputStream *stream, ConfigFile *cf)
{
  Symbol *sym;          /* The symbol-table entry of the new alias */
  unsigned long level;  /* The level-number being named */
/*
 * Get the name to give the following dump-level.
 */
  if(input_literal(stream, 0, "", "", isgraph, "")) {
    input_error(stream, 1, "Missing name for dump-level.\n");
    return 1;
  };
/*
 * Add the level-name to the table of levels.
 */
  sym = new_HashSymbol(cf->levels, stream->work, 0, 0, NULL, 0);
  if(!sym)
    return 1;
/*
 * Now get the level number that was just named.
 */
  if(input_skip_space(stream, 0, 0) ||
     input_ulong(stream, 0, 0, &level) || level < 0 || level > 9) {
    input_error(stream, 1, "Bad level number. Should be in range 0..9.\n");
    return 1;
  };
/*
 * Record the level number along with its name.
 */
  sym->code = level;
  return 0;
}

/*.......................................................................
 * Read the alias name by which the user can refer to a given file-system
 * when specifying which file-systems to backup. The format of the record
 * is the alias-name, followed by the directory where the filesystem is
 * mounted.
 *
 * Input:
 *  stream     InputStream *   The stream to read the list from.
 *  cf          ConfigFile *   The place to record the results.
 * Output:
 *  return             int     0 - OK.
 *                             1 - Error.
 */
static int input_file_system_alias(InputStream *stream, ConfigFile *cf)
{
  Symbol *sym;          /* The symbol-table entry of the new alias */
/*
 * Get the name to give the following file-system.
 */
  if(input_literal(stream, 0, "", "", isgraph, "")) {
    input_error(stream, 1, "Missing name for file-system.\n");
    return 1;
  };
/*
 * Add the file-system name to the corresponding symbol table.
 */
  sym = new_HashSymbol(cf->fs, stream->work, 0, 0, NULL, del_hash_obj);
  if(!sym)
    return 1;
/*
 * Now get the file-system that was just named.
 */
  if(input_skip_space(stream, 0, 0) ||
     input_literal(stream, 0, "", "", isgraph, "")) {
    input_error(stream, 1, "Error reading file-system specification.\n");
    return 1;
  };
/*
 * Record a copy of the file-system specification string.
 */
  sym->data = new_pathname("", stream->work);
  if(!sym->data)
    return 1;
  return 0;
}

/*.......................................................................
 * Read the alias name by which the user can refer to a given backup-device
 * The format of the record is the alias-name, followed by a ufsdump
 * device specification.
 *
 * Input:
 *  stream     InputStream *   The stream to read the list from.
 *  cf          ConfigFile *   The place to record the results.
 * Output:
 *  return             int     0 - OK.
 *                             1 - Error.
 */
static int input_backup_device_alias(InputStream *stream, ConfigFile *cf)
{
  Symbol *sym;          /* The symbol-table entry of the new alias */
/*
 * Get the name to give the following backup device.
 */
  if(input_literal(stream, 0, "", "", isgraph, "")) {
    input_error(stream, 1, "Missing name for backup device.\n");
    return 1;
  };
/*
 * Add the device name to the corresponding symbol table.
 */
  sym = new_HashSymbol(cf->devices, stream->work, 0, 0, NULL, del_hash_obj);
  if(!sym)
    return 1;
/*
 * Now get the device that was just named.
 */
  if(input_skip_space(stream, 0, 0) ||
     input_literal(stream, 0, "", "", isgraph, "")) {
    input_error(stream, 1, "Error reading backup-device specification.\n");
    return 1;
  };
/*
 * Record a copy of the ufsdump device specification string.
 */
  sym->data = malloc(strlen(stream->work) + 1);
  if(!sym->data)
    return 1;
  strcpy((char *)sym->data, stream->work);
  return 0;
}

/*.......................................................................
 * This is a private error-return function of new_ConfigFile().
 */
static ConfigFile *bad_ConfigFile(ConfigFile *cf, InputStream *stream)
{
  (void) del_InputStream(stream);
  return del_ConfigFile(cf);
}

/*.......................................................................
 * Delete a ConfigFile object.
 *
 * Input:
 *  cf     ConfigFile *  The object to be deleted.
 * Output:
 *  return ConfigFile *  The deleted object (ie. NULL).
 */
static ConfigFile *del_ConfigFile(ConfigFile *cf)
{
  if(cf) {
/*
 * Delete the symbol-tables and their contents.
 */
    cf->users = del_HashTable(cf->users);
    cf->levels = del_HashTable(cf->levels);
    cf->fs = del_HashTable(cf->fs);
    cf->devices = del_HashTable(cf->devices);
/*
 * Delete the memory allocated for the above tables.
 */
    cf->mem = del_HashMemory(cf->mem, 1);
/*
 * Delete the container.
 */
    free(cf);
  };
  return NULL;
}

/*.......................................................................
 * This is a wrapper around free(), suitable to be used as a destructor
 * argument of new_HashSymbol().
 */
static SYM_DEL_FN(del_hash_obj)
{
  if(sym_data)
    free(sym_data);
  return NULL;
}

/*.......................................................................
 * Check whether the user that is running this program is authorized to
 * do so.
 *
 * Input:
 *  prog          char *   The name of this program.
 *  cf      ConfigFile *   The contents of the configuration file.
 * Output:
 *  return         int     0 - Not authorized.
 *                         1 - Authorized.
 */
static int user_is_authorized(char *prog, ConfigFile *cf)
{
  uid_t uid;          /* The real user-id of the process */
  struct passwd *pw;  /* The password-file entry associated with uid */
/*
 * Are all users allowed?
 */
  if(cf->everybody)
    return 1;
/*
 * Get the real user-ID of the current process.
 */
  uid = getuid();
/*
 * Get the corresponding password-file entry.
 */
  if(!(pw = getpwuid(uid))) {
    fprintf(stderr, "%s: Error reading password file.\n", prog);
    return 0;
  };
/*
 * Lookup the user in the list of authorized users.
 */
  if(!find_HashSymbol(cf->users, pw->pw_name)) {
    fprintf(stderr, "%s: You are not authorized to use this program.\n",
	    prog);
    return 0;
  };
/*
 * The user is authorized.
 */
  return 1;
}

/*.......................................................................
 * Translate a dump-level name to a ufsdump level number.
 *
 * Input:
 *  prog        char *   The name of this program.
 *  cf    ConfigFile *   The configuration file contents.
 *  name        char *   The name of the level.
 * Output:
 *  return       int     0..9 - A valid dump level.
 *                       -1   - Error.
 */
static int lookup_dump_level(char *prog, ConfigFile *cf, char *name)
{
/*
 * Lookup the level-name designation.
 */
  Symbol *sym = find_HashSymbol(cf->levels, name);
  if(!sym) {
    fprintf(stderr, "%s: Unknown level designation: %s\n", prog, name);
    return -1;
  };
/*
 * Return the dump level that corresponds to the specified name.
 */
  return sym->code;
}

/*.......................................................................
 * Translate the name of a backup device to the corresponding
 * ufsdump device specification.
 *
 * Input:
 *  prog        char *   The name of this program.
 *  cf    ConfigFile *   The configuration file contents.
 *  name        char *   The name of the device.
 * Output:
 *  return      char *   The device specification string, or NULL if
 *                       not found.
 */
static char *lookup_backup_device(char *prog, ConfigFile *cf, char *name)
{
  char *device;   /* The located device specification */
/*
 * Lookup the device.
 */
  Symbol *sym = find_HashSymbol(cf->devices, name);
  if(!sym) {
    fprintf(stderr, "%s: Unknown backup device: %s\n", prog, name);
    return NULL;
  };
/*
 * Extract the device specification.
 */
  device = (char *)sym->data;
/*
 * If the backup-device starts with / then it is a local pathname.
 * Check that this exists.
 */
  if(device[0] == '/') {
    if(test_pathname(device, PATH_ANY, PATH_OK) != NULL) {
      fprintf(stderr, "%s: Invalid backup device (%s).\n", prog,
	      device);
      return NULL;
    };
/*
 * If the device doesn't start with a / then it must be a tape device,
 * specified as either user@host:/... or host:/..., where host can
 * be any of the normal addressing conventions.
 */
  } else {
    char *slash = strchr(device, '/');
    char *colon = strchr(device, ':');
    if(!colon || !slash || slash < colon) {
      fprintf(stderr, "%s: Invalid backup device (%s).\n", prog,
	      device);
      return NULL;
    };
  };
/*
 * Return the device specification.
 */
  return device;
}

/*.......................................................................
 * Translate the name of a file-system to its mount location.
 *
 * Input:
 *  prog        char *   The name of this program.
 *  cf    ConfigFile *   The configuration file contents.
 *  name        char *   The name of the filesystem.
 * Output:
 *  return      char *   The device specification string, or NULL if
 *                       not found.
 */
static char *lookup_filesystem(char *prog, ConfigFile *cf, char *name)
{
/*
 * Lookup the device.
 */
  Symbol *sym = find_HashSymbol(cf->fs, name);
  if(!sym) {
    fprintf(stderr, "%s: Unknown file-system: %s\n", prog, name);
    return NULL;
  };
/*
 * Return the file-system mount point.
 */
  return (char* )sym->data;
}

/*.......................................................................
 * Backup a single file system to a given backup device.
 *
 * Input:
 *  prog       char *   The name of this program.
 *  level       int     The dump level (0..9).
 *  device     char *   The backup device specification string.
 *  fs         char *   The file-system to be backed up.
 *  to_disk     int     If true the backup device names a directory of a
 *                      mounted file system.
 *  fs_alias   char *   The alias name of the filesystem.
 *  host       char *   The name of the current host computer.
 * Output:
 *  return      int     0 - OK.
 *                      1 - Error.
 */
static int backup_file_system(char *prog, int level, char *device, char *fs,
			      int to_disk, char *fs_alias, char *host)
{
  uid_t uid;         /* The owner of this process. */
  char options[81];  /* The option-flags argument of ufsdump */ 
  char *spec;        /* The device specification string */
  char file[256];    /* The name of the backup file where relevent */
  pid_t child;       /* The process ID of the ufsdump process */
  sigset_t newmask;  /* A new signal mask with some signals blocked */
  sigset_t oldmask;  /* The current signal mask */
/*
 * Compile the option-flags argument of ufsdump.
 */
  sprintf(options, "%ducf", level);
/*
 * Compose the backup-device specification string.
 */
  if(to_disk) {
    if(strlen(device) + strlen(host) + strlen(fs_alias) + 9 >= sizeof(file)) {
      fprintf(stderr, "Backup file name too long: %s/%s_%s_dump.%d\n",
	      device, host, fs_alias, level);
      return 1;
    };
    sprintf(file, "%s/%s_%s_dump.%d", device, host, fs_alias, level);
    spec = file;
  } else {
    spec = device;
  };
/*
 * Show the user what we are about to do.
 */
  printf("\n/usr/sbin/ufsdump %s %s %s\n", options, spec, fs);
  fflush(stdout);
/*
 * Record the real-user id of the process before becoming root.
 */
  uid = getuid();
/*
 * Temporarily block all signals that should be forwarded to the
 * ufsdump child processes, and arrange for them to be caught further
 * down when we unblock them.
 */
  sigemptyset(&newmask);
  sigaddset(&newmask, SIGINT);
  sigaddset(&newmask, SIGQUIT);
  sigaddset(&newmask, SIGHUP);
  sigaddset(&newmask, SIGTERM);
  sigaddset(&newmask, SIGCHLD);
  if(sigprocmask(SIG_BLOCK, &newmask, &oldmask) < 0) {
    fprintf(stderr, "%s: sigprocmask: %s\n", prog, strerror(errno));
    return 1;
  };
  signal(SIGINT, signal_catcher);
  signal(SIGQUIT, signal_catcher);
  signal(SIGHUP, signal_catcher);
  signal(SIGTERM, signal_catcher);
  signal(SIGCHLD, signal_catcher);
/*
 * Spawn off a child process to perform the dump.
 */
  child = fork();
/*
 * The child process.
 */
  if(child == 0) {
/*
 * Make the child a session leader so that by sending it signals, we can
 * forward signals both to it and to its own children.
 */
    setpgid(0, 0);
/*
 * Restore signals in the child to their normal defaults.
 */
    signal(SIGINT, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
    signal(SIGHUP, SIG_DFL);
    signal(SIGTERM, SIG_DFL);
    signal(SIGCHLD, SIG_DFL);
/*
 * We need root privileges to get full access to the file-system.
 */
    if(setuid(0)==0) {
      execl("/usr/sbin/ufsdump", "ufsdump", options, spec, fs, (char *)0);
      setuid(uid);
    };
/*
 * If we got here, an error occured.
 */
    perror(prog);
    exit(1);
/*
 * Have the parent backup program wait for the child to complete.
 */
  } else {
    int exit_status;
/*
 * Make the child a session leader so that by sending it signals, we can
 * forward signals both to it and to its own children. Also specify the
 * pid argument to be used with kill(), depending on whether setpgid()
 * succedes or not. Note that EACCES will be returned if the child 
 * calls exec() before this point. This is ok, because the child
 * calls setpgid() itself before the exec. Calling it in both processes
 * ensures that the process group gets set up before it is used by
 * either process.
 */
    int pid_arg = setpgid(child, child) < 0 && errno != EACCES ?
      child : -child;
/*
 * Wait for the child process to exit, or one of the currently
 * blocked signals to be received.
 */
    sigsuspend(&oldmask);
/*
 * If sigsuspend returned due to a signal other than the child exiting,
 * send a SIGTERM signal to the process group of the ufsdump process, then
 * abort.
 */
    if(!caught_sigchld) {
      fprintf(stderr, "%s: Received signal - will attempt to kill ufsdump.\n",
	      prog);
      if(setuid(0)==0) {
	kill(pid_arg, SIGTERM);
	setuid(uid);
      };
    };
/*
 * Get the exit status of the child.
 */
    if(waitpid(child, &exit_status, WNOHANG) != child) {
      fprintf(stderr, "%s: Unable to obtain return status of ufsdump.\n",
	      prog);
      return 1;
    } else if(exit_status != 0) {
      fprintf(stderr, "%s: The ufsdump process failed for %s.\n",
	      prog, fs);
      return 1;
    };
  };
/*
 * Reset the signal mask.
 */
  if(sigprocmask(SIG_SETMASK, &oldmask, NULL) < 0) {
    fprintf(stderr, "%s: sigprocmask: %s\n", prog, strerror(errno));
    return 1;
  };
  return 0;
}

/*.......................................................................
 * This is the signal handler used by backup_file_system() to catch and
 * record signals that should be passed on to the ufsdump child processes.
 */
static void signal_catcher(int sig)
{
  if(sig == SIGCHLD)
    caught_sigchld = 1;
}

