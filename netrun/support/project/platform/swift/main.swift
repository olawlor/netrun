import foo
import NetRun

// Highly Swift-y protocol code generated with help from Gemini 2.5:

// A type that can be created as a function argument
protocol Argumentable {
    static func make_argument() -> Self
}

// A type that can be displayed as a function return value
protocol Returnable {
    static func display_return(_ value: Self)
}

// Teach Int how to be an argument and a return value
extension Int: Argumentable, Returnable {
    static func make_argument() -> Int { return read_input() }
    static func display_return(_ value: Int) { return netrun_handle_ret(value) }
}
extension Float: Argumentable, Returnable {
    static func make_argument() -> Float { return read_float() }
    static func display_return(_ value: Float) { return netrun_handle_ret(value) }
}
extension Double: Argumentable, Returnable {
    static func make_argument() -> Double { return Double(read_float()) }
    static func display_return(_ value: Double) { return netrun_handle_ret(value) }
}
extension String: Argumentable, Returnable {
    static func make_argument() -> String { return read_string() }
    static func display_return(_ value: String) { return netrun_handle_ret(value) }
}

func netrun_call<A,R>(f: (A) -> R) where A: Argumentable, R: Returnable {
	let input = A.make_argument()
	let result = f(input)
	R.display_return(result)
}
// overloads because Swift doesn't let you protocol extension Void (yet!)
func netrun_call<R>(f: () -> R) where R: Returnable {
	let result = f()
	R.display_return(result)
}
func netrun_call<A>(f: (A) -> Void) where A: Argumentable {
	let input = A.make_argument()
	f(input)
}
func netrun_call(f: () -> Void) {
	f()
}


//var v = foo()
//print("Program complete.  Return",v)
netrun_call(f: foo)

