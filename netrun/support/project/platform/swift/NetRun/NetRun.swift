import Foundation

// Primitive NetRun support code library in Swift
func read_input() -> Int {
        print("Please enter an input value:")

        if let ret = Int(readLine()!) {
                print(String(format:"read_input> Returning %ld (0x%lX)",ret,ret))
                return ret
        } else {
                print("read_input> Input format error")
                return -1
        }
}
func read_float() -> Float {
        print("Please enter a float input value:")

        if let ret = Float(readLine()!) {
                print(String(format:"read_float> Returning %f",ret))
                return ret
        } else {
                print("read_float> Input format error")
                return -1
        }
}
func read_string() -> String {
        if let ret = readLine() {
                return ret
        } else {
                print("read_string> Input error")
                return ""
        }
}

func netrun_handle_ret<T>(_ value: T) where T : CustomStringConvertible {
        print("Program complete.  Return '",value,"'");
}
func netrun_handle_ret(_ value : Int) {
        print(String(format:"Program complete.  Return %d (0x%X)",value,value))
}

