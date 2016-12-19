
# Preometheus Script
Prom script is a side project I started working on a few years ago.  I've always been interested in language parsing, and over the years I've gotten a little better with it every step of the way.
Prom script started as a way to alleviate some of my frustrations working with javascript.  Previously, it had been abandoned since there were many other options for transpiled languages, but I decided to pick this back up as more of a thought experiment.  
In the beginning, prom script resembled c style languages pretty closely, but I started to think, what if I didn't constrain myself to behaving like just another c language.  What modern constructs might I mimic?  what fresh ideas could I use to generate code the way I wanted to.
What could I do to increase readibility, or reduce certain mistakes, or to encourage certain design patterns.  This is as far as I've gotten so far.  Let me know if you think it's cool.  

## Declarations

Prom script is typed, and thus uses let to assign types to your variables
```
let foo num
const bar string //very important data, do not change me
```


You can also assign multiple variables at once.  Notice, that Prom will assign the last known value type to all variables
```
let number1, number2 number
```


You can also assign value to your things at the same time
```
let number1, number2, number3 num = 1, 2, 3
```


Note, you have to match type on declaration. Don't worry, you can mix types in a let statement as needed
```
let number1 num, string1 string, bool1 bool = 1, "foo", true
```


For convenience, you can use the declassign operators, which will declare and assign your variables at the same time, inferring type where necessary.  
```
number1, string1 := 100, "bar"
const1 ^= "Very important data"
```

## Functions
Let's say that we want to get some work done with out new super set language.  We're going to need some functions.
```
func foo () {
  return "bar"
}
```
You might notice that out function returns a value, but doesn't specify what type of value will be returned.  That is because function return type can be inferred.  We can also set it explicitly.

```
func foo () : string {
  return "my string bar"
}
```

We can also add in some parameters.  Notice that type inference works here as well
```
func concat ( str1 string, str2 string ) {
  return str1 + str2
}
```

Prom also supports default and rest parameters
```
//notice that type inference works with our default perameter
func add(num1 num, num2 = 2) {
  return num1 + num2
}

func makeArray(...rest) {
  return rest;
}
```

## Helpers
Prom has a few neat helpers that are sugar to speed up common tasks.

Is can be used to determine the type of a variable
```
isBool := true
isBool is bool //returns true
```

If you find yourself moving things back and forth to json a lot, jsonify and stringify will help out a bit
```
myObj = { foo : "bar" }
stringObject := stringify myObj
objObject := jsonify stringObject
```

Decltype return the compile time type of an object as Prom script is currently aware of it
```
myObj = { foo : "bar" }
decltype myObj //returns "object"

myFunc := func xyz () { return "xyz" }
decltype myFunc //returns "() string"
```

## Comments
Comments are boring, and no one uses them, but if you do use them, be aware that prom supports nested comments.
```
/*super trippy/*nested /*commenting*/*/*/
```
I find the very useful when trying to debug some part of a script that's gone awry.
