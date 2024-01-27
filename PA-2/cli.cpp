#include<iostream>
#include<unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstring>
#include <fcntl.h> 
#include <thread>
#include <mutex>

using namespace std;

mutex mutex_obj;

vector<thread> threads;


void writeToParseFile(ofstream & parseFile,string command
,string inputs,string options,string redirection,string backgroundJob){
    
    

    parseFile<<"----------"<<endl;
    parseFile << "Command: " << command << endl;
    parseFile << "Inputs: " << inputs << endl;
    parseFile << "Options: " << options << endl;
    parseFile << "Redirection: " << redirection << endl;
    parseFile << "Background Job: " << backgroundJob << endl;
    parseFile << "----------" << std::endl;

    parseFile.flush();


    
}


void readAndWriteToConsole(int readEndNumber){

    mutex_obj.lock();

    cout<<"----"<<this_thread::get_id()<<endl;


    
    FILE* fileStream = fdopen(readEndNumber, "r");
    char* line = nullptr;
    size_t len = 0;
    

    while (getline(&line, &len, fileStream) != -1) {
        cout << line;;
    }

    free(line); 
    cout<<flush;

    fclose(fileStream);
    close(readEndNumber);
    cout<<"----"<<this_thread::get_id()<<endl;

    mutex_obj.unlock();
    

}


int main()
{

    ofstream parseFile("parse.txt");
    ifstream commandFile("command.txt");
    if (!parseFile.is_open()) {
        return 0; // Return an error code
    }
    if (!commandFile.is_open()) {
        cout << "Error opening file.\n";
        return 0;  // Return an error code
    }
    
    string line;
    //to keep track of the background jobs
    vector<pid_t> pids;
    
    while (getline(commandFile,line)){
        vector<string> tokens;
        istringstream streamString(line);
        string token;
        while(streamString>>token){
            tokens.push_back(token);
        }
        string command,inputs,options,redirection,backgroundJob,file;
        backgroundJob="n";
        redirection="-";
        command=tokens[0];
        if(tokens.size()>1){// we might get only wait as well. Thats why
            for(int i=1;i<tokens.size();i++){
                if(tokens[i].find("-")!=string::npos){
                    options=tokens[i];
                }else if(tokens[i].find(">")!=string::npos || tokens[i].find("<")!=string::npos){
                    redirection=tokens[i];
                    i++;
                    //after that we get filename
                    file=tokens[i];
                }else if(tokens[i].find("&")!=string::npos){
                    backgroundJob="y";
                }else{
                    inputs=tokens[i];
                }

            }
        }
        writeToParseFile(parseFile,command,inputs,options,redirection,backgroundJob);
        
        int fd[2];
        if(redirection !=">" && command!= "wait"){
            //If the process is not going to output to some file,
            //we need a pipe
            
            pipe(fd);
        }

        if(command!="wait"){


            pid_t newProcess= fork();




            if(newProcess==0){
                //Child

                //Setting arguments to be put in execvp argslist
                vector<char *> args;

                char* commandFormatted = new char[command.length()+ 1];
                strcpy(commandFormatted, command.c_str());

                args.push_back(commandFormatted);
                if(options!=""){
                    char* optionsFormatted = new char[options.length()+ 1];
                    strcpy(optionsFormatted, options.c_str());
                    args.push_back(optionsFormatted);

                }
                if(inputs!=""){
                    char* inputsFormatted = new char[inputs.length()+ 1];
                    strcpy(inputsFormatted, inputs.c_str());

                    args.push_back(inputsFormatted);
                }

                const size_t argsSize=args.size()+1;
                    
                char * argsArray[argsSize];
                for(int i=0;i<args.size();i++){
                    argsArray[i]=args[i];
                }
                argsArray[argsSize-1]=NULL;
                    

                
                if(redirection=="-"){
                    

                    //we need to write the outputs to the write end
                    close(fd[0]);
                    dup2(fd[1],STDOUT_FILENO);
                    execvp(argsArray[0], argsArray);

                }else if(redirection==">"){
                    //redirected to output file
                    //no pipe is needed, so pipe is not available 
                    
                   
                    int newlyCreatedFileDesc=open(file.c_str(), O_CREAT|O_WRONLY|O_APPEND,S_IRWXU);
                        
                    dup2(newlyCreatedFileDesc,STDOUT_FILENO);

                    
                        
                    
                    execvp(argsArray[0], argsArray);


                }else{
                    int newlyCreatedFileDesc=open(file.c_str(), O_CREAT|O_RDONLY|O_APPEND,S_IRWXU);


                    //takes input from the file

                    close(fd[0]);

                    dup2(newlyCreatedFileDesc,STDIN_FILENO);
                    //writes it to write end


                    dup2(fd[1],STDOUT_FILENO);
                    
                    execvp(argsArray[0], argsArray);

                }
                            
                
            }else if(newProcess>0){
                //Parent


                if(backgroundJob=="n"){
                    //we need to wait for child since it is not a background job
                    wait(NULL);
                }else{
                    //it is background job so we need to keep track of them
                    //in case wait command is inputted
                    pids.push_back(newProcess);
                }
                
                //parent either waits or adds pid to background jobs list 
                //and continues to its execution by going to the loop again


                if(redirection=="-" || redirection=="<"){
                    close(fd[1]);
                
                    threads.push_back(thread (readAndWriteToConsole,fd[0]));
                }



            }else{
                cout<<"Fork failed"<<endl;
                return 0;
            }
        }else{
            //wait command is obtained.
            //we want children to finish and
            //and their threads to finish
            for(pid_t childId: pids){
                int status;
                waitpid(childId,&status,0);    
            }
            //for all threads in our thread vector
            //we need to wait until they finish
            for(thread &mythread: threads){
                mythread.join();

            }            
            
            //after that, we need to empty the lists
            const int PIDS_SIZE=pids.size();
            const int THREADS_SIZE=threads.size();
            for(int i=0;i<PIDS_SIZE;i++){
                pids.pop_back();
            }
            for(int i=0;i<THREADS_SIZE;i++){
                threads.pop_back();
            }
        }
        

    }

    //When all commands inside "commands.txt" file is processed by the shell
    //we need to wait for threads to join
	
    for(thread &mythread: threads){
        mythread.join();

    }
    
    

    return 0;
}
