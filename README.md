# COMP304-Project1

We implemented shelldon with two types of execution where one is linux commands(in /bin/) and 
the other is executing files in the current path(project path). We check if the command is in a 
predefined list of commands (user files) otherwise we assume it is a linux command and run it. 
The parent and child processes are implemented where the parent waits the child's execution if & 
does not exist in the command. If it exists then the parent continues execution (the shell is usable). 


In part 2, we implemented > and >> and the cout is printed in the files as required. We also implemented 
history as requested where the history is printed with their ids and user can execute last command with ! 
and the i'th command with !i. In code search, we fully implemented recursive, targeted and default versions 
printing the output as specified in the project. 

Our shell plays a song for 1 min at specified time every day. User inputs the command birdakika with two parameters,
hour and the path of the song that is wanted to be played. This command sets a cronjob for specified hour and path.
Our custom command is singasong. This command generates musical notes as texts. Then plays the song that it generated.
