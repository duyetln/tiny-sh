# Tiny SH

## Overview
A tiny shell command interpreter that supports a subset of shell commands.

## Features

  * Simple commands: `cat`, `ls`, `cp`, `sort`, etc...
  * IO redirections: `<`, `>`, `<&`, `>&`, `<>`, `>>`, `>|`.
  * Pipes: `|`.
  * Logicals: `&&`, `||`.
  * Subshell commands.
  * Naive command parallelization: simply join commands to be parallelized with `;;`.

## Usage

```
tinysh [OPTION] [FILE]
```

  * Options:
    - `-p`: print command tree.
    - `-t`: parallelize commands.
    - `-C`: turn no clobber option on (for `>|`).
  * File: shell script file to execute.

## Syntax of shell command subset

```
CHAR =  "a"|"b"|"c"|"d"|"e"|"f"|"g"|"h"|"i"|"j"|"k"|"l"|"m"|"n"|"o"|"p"|"q"|"r"|"s"|"t"|"u"|"v"|"w"|"x"|"y"|"z"
       |"A"|"B"|"C"|"D"|"E"|"F"|"G"|"H"|"I"|"J"|"K"|"L"|"M"|"N"|"O"|"P"|"Q"|"R"|"S"|"T"|"U"|"V"|"W"|"X"|"Y"|"Z"
       |"!"|"%"|"+"|","|"-"|"."|"/"|":"|"@"|"^"|"_"
       |DIGIT;

DIGIT = 0|1|2|3|4|5|6|7|8|9;

WORD = CHAR , { CHAR };

IO_NUMBER = DIGIT , { DIGIT };

IO_REDIRECTION = [ IO_NUMBER ] ,  ( "<" , WORD | "<&" , IO_NUMBER | "<>" , WORD | ">" , WORD | ">&" , IO_NUMBER | ">>" , WORD | ">|" , WORD );

SIMPLE_COMMAND = WORD , { WORD };

COMMAND = ( SIMPLE_COMMAND | SUBSHELL_COMMAND ) , { IO_REDIRECTION };

SUBSHELL_COMMAND = "(" , COMMAND_SEQUENCE , ")";

PIPELINES = COMMAND , { "|" , COMMAND };

LOGICALS = PIPELINES , { ( "&&" | "||" ) , PIPELINES };

COMMAND_SEQUENCE = LOGICALS , { ";" , LOGICALS };

COMPUTATION = COMMAND_SEQUENCE , { ";;" , COMMAND_SEQUENCE };
```

The start symbol is COMPUTATION.
