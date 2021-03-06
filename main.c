#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <wayland-server-core.h>
#include <wlr/types/wlr_box.h>
#include <wlr/util/log.h>
#include <xkbcommon/xkbcommon.h>

#include "globals.h"
#include "server.h"

static bool debug = false;
static const char *msg_str[NMSG] = { "DEBUG", "INFO", "WARNING", "ERROR" };

//------------------------------------------------------------------------
void say(int level, const char* message, ...) {
   if(level==DEBUG && !debug) return;

   char buffer[256];
   va_list args;
   va_start(args, message);
   vsnprintf(buffer, 256, message, args);
   va_end(args);

   printf("SimpleWay [%s]: %s\n", msg_str[level], buffer);

   if(level==ERROR) exit(EXIT_FAILURE);
}

void spawn(char* cmd) {
   char *sh = NULL;
   if(!(sh=getenv("SHELL"))) sh = (char*)"/bin/sh";

   say(INFO, "Spawn %s", cmd);
   pid_t pid = fork();
   if(pid==0){
      pid_t child;
      setsid();
      sigset_t set;
      sigemptyset(&set);
      sigprocmask(SIG_SETMASK, &set, NULL);
      if((child=fork())==0){
         execl(sh, sh, "-c", cmd, (char*) NULL);
         exit(0);
      }
      exit(0);
   }
   waitpid(pid, NULL, 0);
}

void sigchld(int unused) {
   if(signal(SIGCHLD, sigchld) == SIG_ERR)
      say(ERROR, "Can't install SIGCHLD handler!");
   while (0 < waitpid(-1, NULL, WNOHANG));
}

//--- Main function ------------------------------------------------------
int main(int argc, char **argv) {
   char config_file[64] = { '\0' };

   // Parse arguments
   for(int i=1; i<argc; i++){
      char* iarg = argv[i];
      if(!strcmp(iarg, "--config") && ((i+1)<argc)) {
         sprintf(config_file, argv[++i]);
      }
      else if(!strcmp(iarg, "--debug")) {
         debug = true;
      }
      else if(!strcmp(iarg, "--version")) {
         say(INFO, "Version-"VERSION);
         exit(EXIT_SUCCESS);
      }
      else if(!strcmp(iarg, "--help")) {
         say(INFO, "Usage: simpleway [--config file][--debug][--version][--help]");
         exit(EXIT_SUCCESS);
      }
   }
   
   if(config_file[0]=='\0')
      sprintf(config_file, "%s/%s", getenv("HOME"), ".config/simpleway/configrc");

   if(debug) wlr_log_init(WLR_DEBUG, NULL);

   struct simple_config g_config = { 0 };
   readConfiguration(&g_config, config_file);

   struct simple_server g_server = { 0 }; 
   prepareServer(&g_server, &g_config);

   sigchld(0);
   runServer(&g_server);

   cleanupServer(&g_server);

   return EXIT_SUCCESS;
}

