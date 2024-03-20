#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <sstream>
#include <filesystem>
#include <unistd.h>
#include <sys/wait.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <utime.h>
#include <queue>

std::queue<std::pair<int, std::string>> hist;
int histno;
char cwd[1024];

/*----MAIN_LOOP----*/
void loop();

/*---PARSING_FUNCTIONS---*/
std::string sh_readline();
std::vector<std::string> sh_splitline(std::string);
int sh_execute(std::vector<std::string>&);

/*-----BUILT_IN_COMMANDS_IMPLEMENTATION-----*/
void cd(std::vector<std::string>& args){
  // std::cout<<args[0];
    if (args[1].empty()) {
    fprintf(stderr, "lsh: expected argument to \"cd\"\n");
  } else {
    if (chdir(args[1].c_str()) != 0) {
      perror("lsh");
    }
  }
}

void ls(std::vector<std::string>& args){
  // std::cout<<"LS command decteced\n";
  struct dirent *d;
  DIR *dh;
  if(args.size()<1){
    dh = opendir(args[1].c_str());
    while ((d = readdir(dh)) != NULL){
		  if (d->d_name[0] == '.')
			  continue;
		  printf("%s\t", d->d_name);
	  }
  }else{
    dh = opendir(".");
    while ((d = readdir(dh)) != NULL){
		  if (d->d_name[0] == '.')
			  continue;
		  printf("%s  ", d->d_name);
	  }
  }
}

void cp(std::vector<std::string>& args){
  // std::cout<<"CP detected\n";
  if(args.size()<3){
    std::cout<<"Expected two arguments\n";
  }else{
    int Source,Destination,ReadBuffer,WriteBuffer;
    char *buff[1024];
    Source = open(args[1].c_str(),O_RDONLY);
 
	  if(Source == -1){
		  std::cout<<"\nError opening file"<<args[1]<<"errno = "<<errno<<"\n";
		  exit(EXIT_FAILURE);	
	  }

	  Destination = open(args[2].c_str(),O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
 
	  if(Destination == -1){
		  std::cout<<"\nError opening file"<<args[2]<<"errno = "<<errno<<"\n";
		  exit(EXIT_FAILURE);
	  }
 
	
	  while((ReadBuffer = read(Source,buff,1024)) > 0){
		  if(write(Destination,buff,ReadBuffer) != ReadBuffer)
			  perror("\nError in writing data to \n");
	  }
	
	
	  if(close(Source) == -1)
		  perror("\nError in closing file\n");
 
	  if(close(Destination) == -1)
		  perror("\nError in closing file\n");
 
    }
}

void touch(std::vector<std::string>& args){
  std::cout<<"touch detected\n";
  int retvalue;
  if(args.size()<2){
    std::cout<<"Usage::touch <filename>\n";
  }
    retvalue=utime(args[1].c_str(),NULL);
  if(retvalue==0){
    std::cout<<"Timestamp modified\n";
  }else{
    retvalue = open(args[1].c_str(),O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    if(retvalue==0){
      perror("Error creating the file");
    }
  }
}

void history(std::vector<std::string>& args){
  std::queue<std::pair<int, std::string>> histCopy = hist;
  while(!histCopy.empty()){
    std::pair<int, std::string> command = histCopy.front();
    std::cout << command.first << "\t" <<command.second <<"\n";
    histCopy.pop();
  }
}

void clear(std::vector<std::string>& args){
  std::cout<<"\e[1;1H\e[2J";
}

// void mkdir(std::vector<std::string>& args){
//   if(args.size()<2){
//     std::cout<<"Usage::mkdir <filename>\n";
//   }else{
//     if(mkdir)
//   }
// }

void rm(std::vector<std::string>& args){
  if(args.size()<2){
    std::cout<<"Usage::touch <filename>\n";
  }else{
    if(remove(args[1].c_str())==-1){
      std::cerr<<"Error deleting file\n";
    }
  }
}

void sh_exit(std::vector<std::string>& args){
  exit(EXIT_SUCCESS);
}

/*----BUILT_IN_COMMANDS-----*/
std::unordered_map<std::string, void (*)(std::vector<std::string>&)> builtinCommands = {
  std::make_pair((std::string)"cd", cd),
  std::make_pair((std::string)"ls", ls),
  std::make_pair((std::string)"cp", cp),
  std::make_pair((std::string)"touch", touch),
  std::make_pair((std::string)"history", history),
  std::make_pair((std::string)"clear", clear),
  std::make_pair((std::string)"rm", rm),
  std::make_pair((std::string)"exit", sh_exit)
};

void signalhandler(int signal){
  std::cout<<"\n\nExiting shell...\n";
  exit(EXIT_SUCCESS);
}


int main(){
  signal(SIGINT, signalhandler);
  loop();
}

void loop(){
  std::string line;
  std::vector<std::string> args;
  int status;

  do{
    getcwd(cwd,1024);
    std::cout<<"\n"<<cwd<<"$\n> ";
    line = sh_readline();
    args = sh_splitline(line);
    status = sh_execute(args);
  }while(status);

}

std::string sh_readline(){
    std::string command;
    std::getline(std::cin, command);
    hist.push(std::make_pair(histno, command));
    histno++;
    return command;
}

#define DELIMS " \r\n\a"
std::vector<std::string> sh_splitline(std::string line){
  std::string token;
  std::vector<std::string> tokens;
  std::istringstream iss(line);

  while(iss>>token){
    tokens.push_back(token);
  }

  return tokens;
}

int sh_execute(std::vector<std::string>& args){
  if(!args.empty()){
    auto it = builtinCommands.find(args[0]);
    if(it != builtinCommands.end()){
        it->second(args);
    }else{
      pid_t pid = fork();
      if(pid == 0){
        char* command[args.size()+1];
        for(size_t i = 0; i < args.size(); ++i){
          command[i] = const_cast<char*>(args[i].c_str());
        }
        command[args.size()] = nullptr;
        execvp(command[0], command);
        std::cerr << "lsh: Command not found: "<<command[0]<<"\n";
        exit(EXIT_FAILURE);
      }else if(pid < 0){
        std::cerr << "Failed to fork process" << "\n";
        return 0;
      } else {
        int status;
        waitpid(pid, &status, 0);
      }
    }
  }
  return 1;
}