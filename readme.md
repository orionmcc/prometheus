
# Prometheus Script
Prom script is a side project I started working on a few years ago.  I've always been interested in language parsing, and over the years I've gotten a little better with it every step of the way.
Prom script started as a way to alleviate some of my frustrations working with javascript.  Previously, it had been abandoned since there were many other options for transpiled languages, but I decided to pick this back up as more of a thought experiment.  
In the beginning, prom script resembled c style languages pretty closely, but I started to think, what if I didn't constrain myself to behaving like just another c language.  What modern constructs might I mimic?  what fresh ideas could I use to generate code the way I wanted to.
What could I do to increase readibility, or reduce certain mistakes, or to encourage certain design patterns.  This is as far as I've gotten so far.  Let me know if you think it's cool.  

## Declarations

Prom script is typed, and thus uses let to assign types to your variables
```
number1: num = 1; const number2: num = 2; string1: string = "foo"
```

```
number1 := 100, string1 := "bar"
!const1 := "Very important data", !const2 := 129
```

## Numbers
number1 = 1,234  
number1 == 1234

## Functions
Let's say that we want to get some work done with out new super set language.  We're going to need some functions.
```
func foo () {
  return "bar"
}
```
You might notice that our function returns a value, but doesn't specify what type of value will be returned.  That is because function return type can be inferred.  We can also set it explicitly.

```
func foo () : string {
  return "my string bar"
}
```

Now an anonymous function

(num x): {
  return x
}(9)

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

But all of this brings us to one of the coller features of prom, which is 

## Currying
Prom script will natively support currying your functions
```
func add(n1 num, n2 num) { return n1 + n2 }
addTwo := add(2)
four := addTwo(2)
``` 

Make sure to mind your use of default and rest values as those can keep a function from currying
```
func restFunc(val num, ...rest) { return rest }
restFunc(1) //will not curry because the compiler interprets this as an empty rest parameter

func defFunc(val num, def = 2)
defFunc(1) //will not curry because the compiler evaluates this as defFunc(1, 2)
```

## Classes
Prom Script supports a moral formal implementation of Classes

```
class Person
{
}
```

Classes support many of the features you might find familiar from other languages including constructors, methods, and member variables.  These member variables can be access protected so that they are only available privately.  If you do not specify, members default to private access.
```
class Person
{
	public name string
	private age num

	public func getName () {
		return name
	}

	public func setName (newName string) {
		return name = newName
	}

	//a default constructor is provided if you do not specify one
	//class constructors support initializers
	Person(name: string, age: num) { /*do something else*/ }
}
```

Classes can be extended in two ways.  Classes can be extended from other classes, or classes may implement any number of interfaces (covered below).

```
class LazyPerson extends Person
{
	public hobby string
	func doSomething() { return "nope" }

	//an extended class must satisfy it's parent's constructors
	LazyPerson(name string, age num, hobby: string) : Person(name, age)
}
```

Once you have your class defined.  Instantiation of that class looks like this

```
Peter := Person("Peter", 35)
Paul := LazyPerson("Paul", 5, "Not having a job")
```

## Interfaces
Interfaces allow you to declare that a class satisfies certain functionality without implementing that functionality.  By doing this, we gain some of the benefits of multiple inheritence, whilie avoiding some of the pitfalls it introduces.

```
interface Aging 
{
	public func growOld() num
	public func isOld() bool
	public func die() string
}
interface Working 
{
	public func workingHardly() bool
	public func goToWork()  string
	public func payTaxes() num
}
```

A class may implement as many interface as desired

```
class Adult implements Aging, Working
{
	private age num
	private money num

	Adult(age: num = 25, money : num = 200)
	// Adult(age := 25, money := 200) //or with type inference

	public func growOld() num
	public func isOld() bool
	public func die() string
	public func workingHardly() bool
	public func goToWork()  string
	public func payTaxes() num
}
```

Instances of objects that implement classes can be used to satisfy evaluations of that type.

```
Adam := Adult()

func day(Working tiredPerson) {
	return tiredPerson.goToWork() + tiredPerson.payTaxes()
}

func toDay(Aging oldPerson) {
	return oldPerson.growOld() + oldPerson.die()
}

func life(Adult oldTiredPerson) : string {
	return day(oldTiredPerson) + toDay(oldTiredPerson)
}

life(Adam)
```

Classes that implement interfaces can also be used with transitive methods.  Transitive methods are called on an object instance as if that object implemented that function.  Instead, that function is implemented externally, and can be applied to all classes that implement that interface.
```
func Working => work() {
	return goToWork() + payTaxes()
}

func Aging => age() {
	return growOld() + die()
}

func Working, Aging => retire() {
	return workindHardly() && isOld() ? "retire" : "keep working"
}
```

With these transitive methods defined, our code from earlier could be refactored as
```
func life(Adult over18Person) : string {
	return over18Person => work() +  over18Person=> retire() + over18Person=> age()
}

life(Adam)
```

which you may find a little easier to read

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

## Interop and casting
It would be unreasonable to assume that you would switch from all of your favorite js libraries to writing pure prom script, so prom offers a few features designed to ease development.

The `ext` key word can be used to signal to prom script that something follows that it may not be familiar with.
```
ext console.log("hello")
console.log("world")
```

ext tells the compiler to be on the lookout for an expression you might not recognize.  When the compiler encounters variables it does not know, it attempts to assign that variable the type `typeless`.  `Typeless` is the compiler's way of conveying that it has seen and parsed a symbol, but it does not know for sure what the type of that symbol is.

Sometimes prom can make inferences based on how a variable is used.  In the above example, prom interprets `console` as an object, due to the member access, and it interprets that `log` is a member function of the `console` object.  In this way, the second occurence of console.log conforms to proms expectations for `console.log()`.  There is no way for prom to know the exact return type of log, so it interprets the return type as `typeless`

Lets say you bring in a variable, and you know for certain what type a typeless object is.  You can explicitly set the type of an object with the type assign operator
```
ext MyEliteVar : obj
```

This assignment is sticky, and `MyEliteVar` will retain that typing for the rest of the program duration.  What if, however, you want to assert temporarily that a variable is of a certain type.  In this case you would use the static cast operator.

```
let a num, b string
a = b //this returns a type mismatch error
a = b :: num //this works, but why would you want to do this you sicko?
```
NB: you should use this sparingly.  There should not be much need to use this as the types in prom are considerably looser than they are in a classic language like c.

The final inter op feature is that large sections of external code can be declared so that prom will recognize it.  We do this with the declext syntax.
```
declext alert ( string )
declext console {
	log ( ... ) //defaults to void return
	error ( ... )
}
```

This would give us propper type checking with which to use standard javascript functionality.  Further, should you decide that an external piece of software is very improtant to your workflow, this gives a very simple, sparse syntax for gaining access to libraries you need most.

## Comments
Comments are boring, and no one uses them, but if you do use them, be aware that prom supports nested comments.
```
/*super trippy/*nested /*commenting*/*/*/
```
I find the very useful when trying to debug some part of a script that's gone awry.

## Compile Time Execution

```
!ENABLE_TESTS := true

func doThis ( num x ) {
 return x
}

func doThat ( num x ) {
 return null
}

func test ( desc string, testCase func, ...params ) {
  if ENABLE_TESTS {
    run testCase testThisAndThat
  }
}

#run test(
 "When input is 9, the output should be 9",
 (params[x: 0]){
   if doThis(x) != 9 {
     panic "doThis(9) should return 9"
   }
 },
 9
)

#run test(
 "When input is 9, the output should be 10",
 (params[x: 0]): {
   if doThat(x) != 10 {
     panic "doThis(9) should return 9"
   }
 },
 9
)
```
