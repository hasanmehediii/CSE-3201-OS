# Custom Shell in C

## Overview

This project is a simple custom shell written in C.
It accepts user commands, parses input, executes commands, and shows output in the terminal.

## Project Files

- `Terminal.c`: Main source code
- `terminal.sh`: Compile and run script
- `README.md`: Project documentation

## How to Compile

Compile manually with GCC:

```bash
gcc Terminal.c -o terminal
```

Or use the script:

```bash
chmod +x terminal.sh
./terminal.sh
```

## How to Run

After compilation:

```bash
./terminal
```

You should see the prompt:

```text
Mehedi@Terminal>
```

Then enter commands.

## Features Implemented

### Built-in Commands

- `cd`: Change current directory
- `echo`: Print text
- `exit`: Close the shell

### Manually Implemented Commands

- `pwd`: Print current working directory
- `ls`: List directory contents
- `mkdir`: Create a directory
- `touch`: Create a file
- `rm`: Remove a file
- `cp`: Copy a file
- `mv`: Move or rename a file
- `cat`: Display file contents

### External Command Support

Unknown commands are executed using:

- `fork()`
- `execvp()`
- `wait()`

Example commands:

```bash
date
whoami
clear
```

### Other Features

- Continuous command loop
- Command argument parsing
- Basic error handling

## Exit the Shell

```bash
exit
```