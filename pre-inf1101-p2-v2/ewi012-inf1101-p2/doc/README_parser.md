# Readme parser

This is a readme file for the parser function in terminal.

## Usage

Write this in terminal for folder location
```
sh run.sh
``` 

After that, you can write in words and operators as you wish.

For the search application to parse a query, use parantheses encapsuling the query.

For instance, for the word "foo" and "bar", and the set operation you want to use is "&&" (AND), you write the following:
```
(foo && bar)
```
When pressing enter, the documents containing both "foo" and "bar" will be shown in the terminal.

## Arguments

"&&" (AND) - for documents containing both words/sets

"||" (OR) -  for documents that have either one or both words

"&!" (ANDNOT) - for documents that have the LEFT word, but NOT the second word. 

## Contribution
by Erling Heimstad Willassen (ewi012)


