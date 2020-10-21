Ethan Turner ID#915729654	 , Omair Khan	 ID#914135359

#High Level Explanation of Design and Implementation:

##struct Input: 
We created a data structure which kept track of the input the user was entering. We categorized the user input in 4 different ways, we treated them either as commands, arguments, tokens, or files. The other functions we implemented had logic to determine which type of input the user was entering. In addition to those 4 attributes to keep track of type of input we implemented two more atttributes which kept track of whether we redirect input or append input. Which were used to flag whether the input was being redirected or appended.

##void parsePipe: 
`parsePipe()` takes three arguments: an array (`input[]`) of `Input` structs, a
string `raw` that represents the user's raw command-line input, and a pointer to
`commandCount`, initialized in `main()`. Inside, the `raw` input is tokenized
with `strtok()`, using the pipe symbol `|` as a delimiter. For each token, a
temporary struct of type `Input` is created, with the value of the token (the
command and its arguments) copied to the `.cmd` property. The temporary struct
is then inserted in sequential order to the `input` array. This process
increments `commandCount` with each one discovered and continues until no tokens
remain.

##int parseInput: 
This was the function we used to parse our input. The function takes in a pointer to a struct type Input. Intially input from the user is stored all in the command attribute of the Input struct, however we use a tokenizer to place commands into the command attribute and arguments into the argument attribute of the Input struct. The way we determined if something was a command or argument was to implement logic that treats input before a space character was read as the command and anything after the space as the argument(s). We had a different function to handle piping, this function is only used when the user input has no piping.

##void checkRedirect:
`checkRedirect()` is the largest of the functions in the `sshell`. It takes one
argument, a pointer `input` to an `Input` struct. At this point in normal
execution, the command(s) have been parsed into individual arguments. A `for`
loop iterates through every argument and checks it for certain patterns
indicating output redirection with the meta-character `>` or output appending
with `>>`. Specifically, the cases are:

 - Meta-character is in the middle of a command, with no whitespaces
  surrounding
	 - Meta-characters are in the middle of the command, and signify output
     redirection with appending
	 - Meta-character is in the middle of the command, signifying output
     redirection with no appending
 - Meta-character is at the beginning of a token/argument, or is its own
 argument
	 - Meta-character does not have whitespace after it, meaning it is part of the
      file name
	 - Meta-character has whitespace after it, meaning the file name should be the
      next argument
	 - A secondary meta-character is found, indicating output appending
		 -  Meta-character does not have whitespace after it, meaning it is part of
         the file name
		 - Meta-character has whitespace after it, meaning the file name should be
         the next argument
 - Meta-characters are found at the end of a token (file name should be the
     next argument), indicating output appending
 - A single meta-character is found at the end of a token, indicating output
 redirection but not appending (file name should be the next argument)

In each of these cases, the file name is copied from the arguments list to the
corresponding `input.file` property, and the `input.willRedirect`
/`input.willAppend` flags are set respectively. Both the file name and
meta-characters are removed from the arguments list to be processed manually by
the shell later as part of execution. Remaining true arguments are shifted to
the proper indices. `checkRedirect` also handles an input with the
meta-character(s) but no file name, printing the "No output file" message on
`stderr`, setting the `errorFlag` and stopping further execution. The function's
return value is the `errorFlag`.

##void printCmdCompletion: 
This function simply just prints the completed message after a command is executed and completed.

##void executeCommands:
`executeCommands()` is perhaps the most vital part of `sshell`. It accepts three
arguments: an array of `Input` structs (`piping[]`), the `commandCount`, and the
full command input string, known as `message`. A `for` loop iterates through
`piping[]`, with `commandCount` as the index bound. We first check for a `cd`
command, which uses the `chdir()` system call and then prints the command
completion message. If the command does not match `cd`, it enters "normal"
execution, where `pipe()` and `fork()` are used to create child processes and
interprocess communication channels. Depending on the command position in the
array (that is, its position in the user's piped command sequence) input is
either closed or reads from the output of the previous command. Output is also
either piped (using file descriptors) to the next command's input, printed to
the terminal, or redirected/appended to the requested file. The last two cases
are only possible if the command is last in the pipeline. The current command is
then checked for a `pwd` command, which is handled through a built-in call to
`getcwd()` and `printf()`. Other valid commands are initialized using a call to
`execvp()`. The parent process will wait for child commands to complete, storing
their exit statuses in an array and closing remaining file descriptors.
`executeCommands()` will then call `printCmdCompletion()`, forwarding `message`,
the `statusArray` populated by the parent, and the `commandCount`.

##void execSLS:
`execSLS()` is `sshell`'s built-in `ls`-like command that prints a directory's
contents along with its size in bytes. It is implemented by calling `getcwd()`
and `scandir()` to discover files. File information is returned from iterative
calls to `stat()`. The formatted file name and size is then printed to `stdout`,
and `printCmdCompletion()` is called.

##int main: 
The main function is just a while loop that calls the above functions when appropriate. The only time the loop will break is when the user enters 'exit'. After taking the user input we send it in, so that it is parsed, once it is parsed, we keep track of how many commands are in the user input and iterate through our array of commands. We iterate through it using a for loop followed by a series of if conditions that check to see if either certain commands have certain characters or flags that indicate what type of command it is. If the command meets the condition of the if statement then it is passed to the appropriate function. This process repeats until the user enters 'exit' as tge user input.

#Testing:

- We used the tester provided by the professor to do some basic testing.
- We added some more tests to the tester to do more rigorous testing.
- We completed many manual sorts of testing and rigrously tested all the commands especially piplining. 

#Sources:
- stackoverflow.com
- geeksforgeeks.com
- GNU.org
- tutorialspoint.com
- cplusplus.com
- Joel Porquet's 150 Lectures and Slideshows