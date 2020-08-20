/*
  A deeply templated big number class, *only* useful when
  the magnitude of the numbers is known.
  
  This is often the case in cryptography.
  
  Dr. Orion Lawlor, lawlor@alaska.edu, 2015-02-05 (Public Domain)
*/
#ifndef __OSL__BIGNUM_H
#define __OSL__BIGNUM_H

#include <iostream> /* for cout */
#include <iomanip> /* for setbase */
#include <stdlib.h> /* for abort() */

#ifdef _WIN32 /* on Windows, use predefined __uint64 */
   typedef unsigned __int32 uint32_t;
   typedef unsigned __int64 uint64_t;
#else /* C99 supports "stdint" */
#  include <stdint.h> /* for uint64_t and friends */
#endif

/** Paranoia levels:
   level 0: all code is assumed to work properly (no self-checking)
   level 1: check for user-caused errors
   level 2: check for library-caused errors
   level 3 (TODO): add "woop" value to check arithmetic
 */
#define OBIGNUM_PARANOIA 2

/** Self-check failed! */
template <class T,class T2>
inline void obignum_die(const char *where,const T &expected,const T2 &actual)
{
	std::cout<<"FATAL ERROR in obignum library: "<<where<<"\n";
	std::cout<<"Expected:	"; expected.printHex();
	std::cout<<"Actual:  	"; actual.printHex();
	abort();
}

/** Simple metaprogramming utility functions */

/** Clamp iteration count at zero, avoiding problems with negative values. 
template <int N,int POSITIVE> struct ZEROCLAMP_CMP { enum { value=N }; };
template <int N> struct ZEROCLAMP_CMP<N,0> { enum { value=0 }; };
template <int N> struct ZEROCLAMP : public ZEROCLAMP_CMP<N, (N>0) > {};
*/
template <int N> struct ZEROCLAMP { enum {value=(N>0)?N:0}; };


/** Compile-time maximum / minimum */
template <int N,int M> struct MAXVAL { enum {value=(N>M)?N:M}; };
template <int N,int M> struct MINVAL { enum {value=(N<M)?N:M}; };



/* FORLOOP template metaprogramming system
  This code is useful when the iteration count is a known
  compile-time constant.
  Rationale: we want to unroll short loops completely,
  and unroll a few iterations even for big loops. 
  Compilers are really bad about doing this automatically,
  but if you use the template magic, they're amazingly good at
  constant propagation, assigning array indices to registers, 
  and other fast stuff.
*/

/** This version implements a loop with classic iteration. 
    This is normally the slowest version, but has good code size. */
template <class BODY,int ITERATIONS>
struct FORLOOP_LOOP {
	static inline void run(BODY &body,int first_iteration=0) {
		for (int i=0;i<ITERATIONS;i++)
			body.run(first_iteration+i);
	}
};


/** This version completely unrolls the loop.
  The code size is bigger, but it allows the optimizer
  access to everything when the loop body is simple. */
template <class BODY,int ITERATIONS>
struct FORLOOP_FULL {
	static inline void run(BODY &body,int first_iteration=0) {
		body.run(first_iteration);
		// we recurse with one less iteration count
		FORLOOP_FULL<BODY,ZEROCLAMP<ITERATIONS-1>::value >
			::run(body,1+first_iteration);
	}
};
/// Partial specialization for iteration count of zero.  
///  This template is the base case for the recursion.
template <class BODY> 
struct FORLOOP_FULL<BODY,0> {
	static inline void run(BODY &body,int first_iteration) 
	{ /* for zero iterations, do nothing. */ }
};

/**
  This version unrolls the loop into segments of size UNROLL.
  This is a good balance between the two above implementations.
*/
template <class BODY,int ITERATIONS,int UNROLL=2>
struct FORLOOP_UNROLL {
	static inline void run(BODY &body,int first_iteration=0) {
		int i;
		for (i=0;i+UNROLL-1<ITERATIONS;i+=UNROLL) {
			FORLOOP_FULL<BODY,UNROLL>::run(body,first_iteration+i);
		}
		// cleanup loop
		FORLOOP_FULL<BODY,ITERATIONS%UNROLL >::run(body,first_iteration+i);
	}
};


/** 
  This comparator decides between partial and full loop unrolling.
*/
template <class BODY,int ITERATIONS,int USE_FULL> struct FORLOOP_SMARTUNROLL_CMP {};
template <class BODY,int ITERATIONS> 
	struct FORLOOP_SMARTUNROLL_CMP<BODY,ITERATIONS,0> 
		: public FORLOOP_FULL<BODY,ITERATIONS> {};
template <class BODY,int ITERATIONS> 
	struct FORLOOP_SMARTUNROLL_CMP<BODY,ITERATIONS,1> 
		: public FORLOOP_UNROLL<BODY,ITERATIONS> {};

/**
  If ITERATIONS <= FULLCOUNT, fully unroll the loop.
  Otherwise only unroll a few iterations at a time.
*/
template <class BODY,int ITERATIONS,int FULLCOUNT>
	struct FORLOOP_SMARTUNROLL
		: public FORLOOP_SMARTUNROLL_CMP<BODY,ITERATIONS,(ITERATIONS>FULLCOUNT) > {};



/************** Multi-Precision Arithmetic Support **************/
/**
  "limb_sizes" describes the data sizes used to represent "limbs",
  the atomic pieces of our big numbers.
*/
struct limb_sizes_intlong {
	// A "limb" is one atomic piece of our big number.
	typedef uint32_t limb_t;
	// A signed limb.
	typedef int32_t slimb_t;
	// A "limb2" is at least twice as big as one limb.
	typedef uint64_t limb2_t;
};

/**
  "limb_ops" performs single operations on the limbs.
*/
template <class limb_sizes>
struct limb_ops
{
	typedef typename limb_sizes::limb_t limb_t;
	typedef typename limb_sizes::limb2_t limb2_t;
	enum {limb_bits=8*sizeof(limb_t)};

	// Extract the low bits of this limb2:
	static inline limb_t lowbits(const limb2_t L) { 
		return (limb_t)L;
	}
	// Extract the high bits of this limb2:
	static inline limb_t highbits(const limb2_t L) { 
		return (limb_t)(L>>limb_bits);
	}
	
	// Full adder: add with carry.
	//  Returns high bits of output carry.
	template <int signA,int signB>
	static inline limb_t add(limb_t *dest,limb_t A,limb_t B,limb_t carryIn)
	{
		limb2_t ap=A; // promote both source operands to limb2
		limb2_t bp=B;
		
		limb2_t sum=ap; // full sum: N+2 bits
		if (signB==+1) sum+=bp+carryIn;
		if (signB==-1) sum+=-bp+(typename limb_sizes::slimb_t)carryIn;
		
		*dest=lowbits(sum); // low bits only
		limb_t carryOut=highbits(sum); // high bits only
	//	std::cout<<std::hex<<" Add: "<<ap<<(signB==+1?" + ":" - ")<<bp<<
	//		" + "<<carryIn<<" => "<<*dest<<" + "<<carryOut<<"\n";
		return carryOut;
	}

	// Full multiplier: multiply with carry.
	//  Returns high bits of output carry.
	static inline limb_t mul(limb_t *dest,limb_t A,limb_t B,limb_t carryIn)
	{
		limb2_t ap=A; // promote both source operands to limb2
		limb2_t bp=B;
		limb2_t prod=ap*bp; // full product: 2*N bits
		limb_t carryOut=add<1,1>(dest,*dest,lowbits(prod),carryIn);
		limb_t newOut=carryOut+highbits(prod); 

	#if OBIGNUM_PARANOIA>=2
		// I think newOut can't possibly carry, but I'm checking anyway:
		if (newOut<carryOut) {
			std::cout<<"fullMul unexpected carry! A="<<ap<<"  B="<<bp<<"  prod="<<prod<<" carryIn="<<carryIn<<" carryOut="<<carryOut<<" newOut="<<newOut<<"\n";
		}
	#endif
		return newOut;
	}
};

typedef limb_ops<limb_sizes_intlong> limbtraits_default;


/**
  "numtraits" describes how big numbers are represented,
  and how errors like overflow are handled.
*/
template <typename limbtraits_>
struct numtraits_default {
	typedef limbtraits_ limbtraits;
	typedef typename limbtraits::limb_t limb_t;
	
	/* Overflow policy: by default, we ignore overflow, resulting in wraparound.
	   You could also throw an exception or print an error here. */
	struct overflow {
		// This function is called when truncating nonzero data
		static inline void trim_nonzero(limb_t theValue) {}
		
		// This function is called when there is a carry out of an add
		static inline void add_carry(limb_t theCarry=0) {}
		// This function is called when there is a carry out of a subtract
		static inline void sub_underflow(limb_t theCarry=0) {}
		
		// This function is called when there is a carry out of a multiply.
		static inline void mul_carry(limb_t theCarry) {}		
	};
};

/** 
  "limb_array" stores a fixed-size array of limbs.
  It's the passive storage base for the bignum class, 
  which exposes an active interface with arithmetic and such.
*/
template <int BITCOUNT, typename limbtraits=limbtraits_default >
class limb_array {
public:
	// Copy over names from the traits:
	typedef limbtraits traits;
	typedef typename limbtraits::limb_t limb_t;
	enum { limb_bits = 8*sizeof(limb_t) }; // bits per limb
	enum { NBIT=BITCOUNT }; // significant bits total
	enum { NLIMB=(BITCOUNT+limb_bits-1)/limb_bits }; // number of limbs (round up)
	
	/**
	  This stores the pieces of our number, the "limbs".
	  Limbs are stored starting at the little end,
	   so limb[0] is the lowest-value number.
	*/
	limb_t limb[NLIMB];
};


/* These are the underlying arithmetic operations on limbs. 
  The "run" methods do a single iteration of work, and are called using the 
  FORLOOP template magic above, by the bignum classes below.
*/

// Zero out this data (iteration class)
template <class L>
class zeroLimb {
	typename L::limb_t *dest;
public:
	// Simple constructor: zero everything in this class
	inline zeroLimb(L &dest_) :dest(dest_.limb) {
		FORLOOP_SMARTUNROLL<zeroLimb,L::NLIMB,8>::run(*this);
	}
	// More complex run-separate constructor
	inline zeroLimb(typename L::limb_t *dest_) :dest(dest_) {}
	
	inline void run(int i) { dest[i]=0; }
};


// Copy limbs between these two arrays
template <class D,class S>
class copyLimb {
	typename D::limb_t *dest;
	const typename S::limb_t *src;
public:
	inline copyLimb(D &dest_,const S &src_) 
		:dest(dest_.limb),src(src_.limb) 
	{
		if ((int)D::NLIMB<=(int)S::NLIMB) 
		{ // source has enough to cover entire destination
			FORLOOP_SMARTUNROLL<copyLimb,D::NLIMB,16>::run(*this);
		} 
		else 
		{ // dest is longer--first copy what we can
			FORLOOP_SMARTUNROLL<copyLimb,S::NLIMB,8>::run(*this);

			// now zero-fill rest of array
			typedef zeroLimb<D> zero_t;
			zero_t zero(dest+S::NLIMB);
			FORLOOP_SMARTUNROLL<zero_t,D::NLIMB-S::NLIMB,8>::run(zero,0);
		}
	}
	inline void run(int i) { dest[i]=src[i]; }
};



// Return -1 if A<B, 0 if A==B, +1 if A>B (like strcmp)
// Low-to-high comparison order is easier to implement and less side-channel sensitive.
//  Subtle: the last comparison will be at the highest-order limb.
//  This will look through all the limbs low-to-high anyway.
template <class A,class B>
class cmpLimb {
	const typename A::limb_t *Alimb;
	const typename B::limb_t *Blimb;
public:
	int cmp;
	inline cmpLimb(const A &A_,const B &B_) 
		:Alimb(A_.limb),Blimb(B_.limb),cmp(0) 
	{
	}
	inline void run(int i) { 
		if (Alimb[i]<Blimb[i]) cmp=-1;
		if (Alimb[i]>Blimb[i]) cmp=+1;
		// else cmp stays at zero
	}
};

// Simpler function wrapper interface
template <class A,class B>
int cmp2(const A &A_,const B &B_) {
	typedef cmpLimb<A,B> loop_t;
	loop_t loop(A_,B_);
	// check common limbs:
	FORLOOP_LOOP<loop_t,MINVAL<A::NLIMB,B::NLIMB>::value >::run(loop); 
	
	// check additional limbs beyond end of other value:
	int cmp=loop.cmp;
	if ((int)A::NLIMB<=(int)B::NLIMB) { /* B is longer */
		for (int i=A::NLIMB; i<B::NLIMB; ++i)
			if (B_.limb[i]>0) // B has a value, where A is implicitly zero
				cmp=-1; // B is bigger
	} 
	else /* A is longer */
	{
		for (int i=B::NLIMB; i<A::NLIMB; ++i)
			if (A_.limb[i]>0) // A has a value, where B is implicitly zero
				cmp=+1; // A is bigger
	}
	return cmp;
}

// Bit shift left
template <class D, class A>
class shiftLeft {
	typename D::limb_t *dest;
	const typename A::limb_t *Alimb;
public:
	int bitsShift; // bit shift count
	int limbShift; // limb shift count
	inline shiftLeft(D &dest_,const A &A_,int bitsShift_) 
		:dest(dest_.limb),Alimb(A_.limb),bitsShift(bitsShift_)
	{
		limbShift=bitsShift/D::limb_bits; // shift in limbs (rounded down)
		bitsShift -= limbShift*D::limb_bits; // leftover per-limb shift
		FORLOOP_SMARTUNROLL<shiftLeft,D::NLIMB,16>::run(*this);
	}
	inline void run(int Di) { 
		typename D::limb_t out=0;
		int Ai=Di-limbShift;
		if (Ai>=0 && Ai<A::NLIMB) out=Alimb[Ai]<<bitsShift;
		Ai--; // grab high bits from preceeding index
		if (bitsShift!=0 && Ai>=0 && Ai<A::NLIMB) out|=Alimb[Ai]>>(D::limb_bits-bitsShift);
		dest[Di]=out;
	}
};

// Bit shift right
template <class D, class A>
class shiftRight {
	typename D::limb_t *dest;
	const typename A::limb_t *Alimb;
public:
	int bitsShift; // bit shift count
	int limbShift; // limb shift count
	inline shiftRight(D &dest_,const A &A_,int bitsShift_) 
		:dest(dest_.limb),Alimb(A_.limb),bitsShift(bitsShift_)
	{
		limbShift=bitsShift/D::limb_bits; // shift in limbs (rounded down)
		bitsShift -= limbShift*D::limb_bits; // leftover per-limb shift
		FORLOOP_SMARTUNROLL<shiftRight,D::NLIMB,16>::run(*this);
	}
	inline void run(int Di) { 
		typename D::limb_t out=0;
		int Ai=Di+limbShift;
		if (Ai>=0 && Ai<A::NLIMB) out=Alimb[Ai]>>bitsShift;
		Ai++; // grab high bits from next index
		if (bitsShift!=0 && Ai>=0 && Ai<A::NLIMB) out|=Alimb[Ai]<<(D::limb_bits-bitsShift);
		dest[Di]=out;
	}
};

// Ripple-carry three-operand addition or subtraction, depending on signs.
//   signA==1 and signB==1 is normal addition
//   signA==1 and signB==-1 is subtraction
//   signA==-1 and signB==1 is reversed subtraction
//   signA==-1 and signB==-1 is negate and add (!?)
template <int signA,int signB,class D, class A,class B>
class addLimb3 {
	typename D::limb_t *dest;
	const typename A::limb_t *Alimb;
	const typename B::limb_t *Blimb;
public:
	typename D::limb_t carry;
	inline addLimb3(D &dest_,const A &A_,const B &B_,typename D::limb_t carryIn=0) 
		:dest(dest_.limb),Alimb(A_.limb),Blimb(B_.limb),carry(carryIn) 
	{
		// bounds checking is inside "run": we always 
		//   loop over the whole destination, to drag carries through.
		FORLOOP_SMARTUNROLL<addLimb3,D::NLIMB,16>::run(*this);
		// FORLOOP_LOOP<addLimb3,D::NLIMB>::run(*this);
	}
	inline void run(int i) 
	{ 
		carry=D::traits::template add<signA,signB>(dest+i, 
			(i<A::NLIMB)?Alimb[i]:0, // bounds check
			(i<B::NLIMB)?Blimb[i]:0,
			carry); 
	}
};

//  Signed addition or subtraction.  Returns the carry out.
template <int signA,int signB,class D, class A,class B>
inline typename D::limb_t addsub3(D &dest_,const A &A_,const B &B_,typename D::limb_t carryIn=0)
{
	addLimb3<signA,signB,D,A,B> loop(dest_,A_,B_,0);
	return loop.carry;
}
// Simple wrapper function, to avoid manually specifying template parameters
template <class D, class A,class B>
inline typename D::limb_t add3(D &dest_,const A &A_,const B &B_,typename D::limb_t carryIn=0)
{
	return addsub3<1,1>(dest_,A_,B_,carryIn);
}
template <class D, class A,class B>
inline typename D::limb_t sub3(D &dest_,const A &A_,const B &B_,typename D::limb_t carryIn=0)
{
	return addsub3<1,-1>(dest_,A_,B_,carryIn);
}


// Ripple-carry one-operand addition (e.g., increment)
template <class D,int sign>
class addLimb {
	typename D::limb_t *dest;
public:
	typename D::limb_t carry;
	inline addLimb(D &dest_,typename D::limb_t carryIn=0) 
		:dest(dest_.limb),carry(carryIn) 
	{
		FORLOOP_SMARTUNROLL<addLimb,D::NLIMB,16>::run(*this);
	}
	inline void run(int i) { 
		carry=D::traits::template add<1,sign>(dest+i, dest[i],carry,0); 
	}
};

// Inner loop of multiplication:
//   Multiply long A by scalar Bj, and add into D.
template <class D, class A> 
class mulInner {
private:
	typename D::limb_t *destj;
	const typename A::limb_t *Alimb, Bj;
public:
	typename D::limb_t carry;
	inline mulInner(typename D::limb_t *destj_,const typename A::limb_t *A_,const typename A::limb_t Bj_) 
		:destj(destj_),Alimb(A_),Bj(Bj_)
	{
		carry=0;
	}
	inline void run(int i) { 
		// essentially dest[j+i] += A[i]*B[j]  (with carries)
		carry = D::traits::mul(destj+i,Alimb[i],Bj,carry);
	}
};

// Multiplication via simple 'schoolbook' nested loops method.
//   Assumptions: dest is initialized to zero, and does not alias A or B.
template <class D,class A,class B> 
class mulOuter {
private:
	typename D::limb_t *dest;
	const typename A::limb_t *Alimb;
	const typename B::limb_t *Blimb;
public:
	inline mulOuter(D &dest_,const A &A_,const B &B_) 
		:dest(dest_.limb),Alimb(A_.limb),Blimb(B_.limb) 
	{
		// FIXME: for large D::NLIMB, use Karatsuba divide-and-conquer multiplication
		
		/* Equivalent algorithm before unrolling: 
			for (int j=0;j<B::NLIMB;j++) {
				limb2_t carry=0;
				for (int i=0;i<A::NLIMB;i++) {
					carry=mul(dest[j+i],A[i],B[j],carry);
				}
			}
		*/
		FORLOOP_LOOP<mulOuter,B::NLIMB>::run(*this);
	}
	inline void run(int j) { 
		typedef mulInner<D,A> body;
		body b(dest+j,Alimb,Blimb[j]);
		FORLOOP_SMARTUNROLL<body,A::NLIMB,8>::run(b);
		if (b.carry!=0) {
			int dj=j+A::NLIMB;
			if (dj<D::NLIMB) dest[dj]+=b.carry;
			else  D::TRAITS::overflow::mul_carry(b.carry);
		}
	}
};



// Simple wrapper for multiply above.  We must have dest::NLIMB >= A::NLIMB + B::NLIMB,
//   dest cannot alias A or B.
template <class D, class A,class B>
inline void mul(D &dest,const A &a, const B &b)
{
	mulOuter<D,A,B> loop(dest,a,b);
}

// Forward declare division (needs binding below)
template <class Q,class R,class N,class D>
void divrem(Q &quotient,R &remainder,const N &numerator,const D &denom);


/* Return true if this bit number is true */
template <class bigint>
inline bool bit_is_set(const bigint &b,unsigned int bitnumber) {
	if (bitnumber>=bigint::bitcount)
		return false;
	return b.limb[bitnumber/bigint::limb_bits] // entry in limb array
		&
		(1<<(bitnumber&(bigint::limb_bits-1))); // mask within limb value
}


/** Precompute table of squares for fast modular exponentiation */
template <class bigint>
class modular_exponentiation_table {
public:
	const bigint &prime;
	bigint squares[bigint::bitcount]; // == generator^(1<<i)
	
	modular_exponentiation_table(const bigint &generator, const bigint &prime_) 
		:prime(prime_)
	{
		bigint doubling=generator;
		for (int i=0;i<bigint::bitcount;i++) {
			squares[i] = doubling;
			doubling=(doubling*doubling).mod(prime);
		}
	}
	
	/* Raise generator to this power, modulo this prime.
		Returns generator.powmod(powby, prime);
		but at least 2x faster than computing it from scratch.
	*/
	bigint power(bigint powby) const {
		bigint ret(1);
		for (unsigned int i=0;i<bigint::bitcount;i++) {
			if (bit_is_set(powby,i)) ret=(ret*squares[i]).mod(prime);
		}
		return ret;
	}
};



/**
 This represents an unsigned big number, with a compile-time bit count.
 We round up the bit count to the number of limbs, so a bignum<30> and bignum<31>
 are the same internally.
*/
template <int BITCOUNT, typename numtraits=numtraits_default<limbtraits_default> >
class bignum : public limb_array<BITCOUNT,typename numtraits::limbtraits> {
public:
	typedef numtraits TRAITS;
	typedef typename numtraits::limbtraits traits;
	
	// copy over names and constants from templated parent
	typedef limb_array<BITCOUNT,typename numtraits::limbtraits> limb_array_t;
	typedef typename limb_array_t::limb_t limb_t; // type of our pieces
	using limb_array_t::limb; // array of atomic pieces, of type limb_t
	enum {NLIMB=limb_array_t::NLIMB}; // size of array
	enum {limb_bits=traits::limb_bits};
	enum {bitcount=BITCOUNT};
	
#if OBIGNUM_PARANOIA>=2
	// Add memory-smashing check "canary"
	int canary;
	enum{ canary_value=3 };
	~bignum() {
		if (canary!=canary_value) obignum_die("Canary destroyed: memory overwritten!",bignum(canary_value),bignum(canary));
	}
#endif

	// Zero this value
	void zero(void) {
		canary=3;
		zeroLimb<bignum> loop(*this);
	}

	bignum() { zero(); }
	
	// Create from a single low-order limb.
	//  This is how you make constants like 7.
	inline bignum(limb_t value) {
		zero();
		limb[0]=value;
	}
	
	/// Return true if this is an even number
	bool is_even() const { return (limb[0]&1)==0; }
	bool is_odd() const { return (limb[0]&1)==1; }
	
	// Return true if this looks like a hex string, starting with "0x":
	bool stringStartsHex(const std::string &str) const {
		return str.size()>=3 && str[0]=='0' && str[1]=='x';
	}
	
	// Create from a decimal or hex (0x) string.
	bignum(const std::string &str) {
		if (stringStartsHex(str))
			setHex(str.substr(2));
		else
			setDecimal(str);
	}

	/// Set our value by parsing a string.  This is how you get big numbers in.
	bool setDecimal(const std::string &str) { return setBase(str,10); }
	/// Set our value by parsing a hex string, with or without leading 0x:
	bool setHex(const std::string &str) { 
		if (stringStartsHex(str))
			return setBase(str.substr(2),16); 
		else
			return setBase(str,16); 
	}
	
	/// Parse this string as representing numbers in this base, from 2 (binary) to 37
	///  Ignores uppercase and lowercase.
	bool setBase(const std::string &str,int base) {
		*this = 0;
		bignum<1> baseBig=base;
		for (int place=0;place<(int)str.size();++place) {
			char c=str[place]; // read char from string
			
		// Convert char to integer
			int value=base;
			if (c>='0' && c<='9') value=c-'0'; // digit
			else if (c>='a' && c<='z') value=c-'a'+10; // lowercase
			else if (c>='A' && c<='Z') value=c-'A'+10; // uppercase
			else if (c==' ' || c==':') continue; // skip over spaces and colons
			if (value>=base) return false; // bad char in string
		
		// Add place-value to existing number
			if (place>0) *this *=baseBig;
			*this +=value;
		}
		return true;
	}
	
	// Truncate (trim) to this many bits. 
	//  Rounds up to an even number of limbs.
	template <int NEWCOUNT>
	inline bignum<NEWCOUNT> trim(void) const {
		typedef bignum<NEWCOUNT> ret_t;
		ret_t ret;
		copyLimb<ret_t,bignum> loop(ret,*this);
		
		// Check for error during truncation
		for (int i=ret_t::NLIMB;i<NLIMB;++i)
			if (limb[i]!=0)
				numtraits::overflow::trim_nonzero(limb[i]);
		
		return ret;
	}
	
// Arithmetic

	// Friend operations: these look convenient, but the separate return value makes them inefficient,
	//   and the size needs to grow to contain carry bits.
	template <int B_BITCOUNT,typename TRAITS>
	inline bignum<(1+MAXVAL<BITCOUNT,B_BITCOUNT>::value),TRAITS> 
	operator+(const bignum<B_BITCOUNT,TRAITS> &B) const
	{
		bignum<(1+MAXVAL<BITCOUNT,B_BITCOUNT>::value),TRAITS> ret;
		if (add3(ret,*this,B)!=0)
			TRAITS::overflow::add_carry(); // what?!  impossible!
		return ret;
	}
	
	template <int B_BITCOUNT,typename TRAITS>
	inline bignum<(1+MAXVAL<BITCOUNT,B_BITCOUNT>::value),TRAITS> 
	operator-(const bignum<B_BITCOUNT,TRAITS> &B) const
	{
		bignum<(1+MAXVAL<BITCOUNT,B_BITCOUNT>::value),TRAITS> ret;
		if (sub3(ret,*this,B)!=0)
			TRAITS::overflow::sub_underflow();
		return ret;
	}
	template <int B_BITCOUNT,typename TRAITS>
	inline bignum<(BITCOUNT+B_BITCOUNT),TRAITS> 
	operator*(const bignum<B_BITCOUNT,TRAITS> &B) const
	{
		bignum<(BITCOUNT+B_BITCOUNT),TRAITS> ret;
		mul(ret,*this,B);
		return ret;
	}
	
	// These two-operand versions keep the size constant, and are recommended!
	// Self-addition.
	template <int B_BITCOUNT,typename B_TRAITS> inline bignum &
	operator+=(const bignum<B_BITCOUNT,B_TRAITS> &b) 
	{
		if (add3(*this,*this,b)!=0)
			numtraits::overflow::add_carry();
		return *this;
	}
	// Self-subtraction.
	template <int B_BITCOUNT,typename B_TRAITS> inline bignum &
	operator-=(const bignum<B_BITCOUNT,B_TRAITS> &b) 
	{
		if (sub3(*this,*this,b)!=0)
			numtraits::overflow::sub_underflow();
		return *this;
	}
	// Self-multiply.  This creates a bigger result, then trims.
	template <int B_BITCOUNT,typename B_TRAITS> inline bignum &
	operator*=(const bignum<B_BITCOUNT,B_TRAITS> &b) 
	{
		bignum<BITCOUNT+B_BITCOUNT> dest(0); // make zero-initialized space for full product
		mul(dest,*this,b);
		*this = dest. template trim<BITCOUNT>(); // copy out low bits
		return *this;
	}

	
	// Increment
	inline bignum &operator++() // prefix ++ (don't use postfix, it creates a copy)
	{
		addLimb<bignum,1> loop(*this,1);
		if (loop.carry!=0)
			numtraits::overflow::add_carry(loop.carry);
		return *this;
	}
	
	// Decrement
	inline bignum &operator--() // prefix -- (don't use postfix, it creates a copy)
	{
		addLimb<bignum,-1> loop(*this,1); 
		if (loop.carry!=0) // we decremented zero!
			numtraits::overflow::sub_underflow(loop.carry);
		return *this;
	}
	
	// Bit shift left
	inline bignum operator<<(int shiftCount) const { 
		bignum ret;
		shiftLeft<bignum,bignum> loop(ret,*this,shiftCount);
		return ret;
	}
	inline bignum operator>>(int shiftCount) const { 
		bignum ret;
		shiftRight<bignum,bignum> loop(ret,*this,shiftCount);
		return ret;
	}
	
	
	// Divide and mod operations
	template <int D_BITCOUNT,typename TRAITS>
	bignum &operator/=(const bignum<D_BITCOUNT,TRAITS> &denom) { 
		bignum quot,rem;
		divrem(quot,rem,*this,denom);
		*this=quot;
		return *this; 
	}
	template <int D_BITCOUNT,typename TRAITS>
	bignum operator/(const bignum<D_BITCOUNT,TRAITS> &denom) const { 
		bignum quot,rem;
		divrem(quot,rem,*this,denom);
		return quot; 
	}
	template <int D_BITCOUNT,typename TRAITS>
	bignum &operator%=(const bignum<D_BITCOUNT,TRAITS> &denom) { 
		bignum quot,rem;
		divrem(quot,rem,*this,denom);
		*this=rem;
		return *this; 
	}
	template <int D_BITCOUNT,typename TRAITS>
	bignum operator%(const bignum<D_BITCOUNT,TRAITS> &denom) const { 
		bignum quot,rem;
		divrem(quot,rem,*this,denom);
		return rem; 
	}

	// Scalar helpers, creating a 1-wide bignum
	inline bignum &operator+=(const limb_t &b) { return *this += bignum<limb_bits>(b); }
	inline bignum &operator-=(const limb_t &b) { return *this -= bignum<limb_bits>(b); }
	inline bignum &operator*=(const limb_t &b) { return *this *= bignum<limb_bits>(b); }
	inline bignum &operator/=(const limb_t &b) { return *this /= bignum<limb_bits>(b); }
	inline bignum &operator%=(const limb_t &b) { return *this %= bignum<limb_bits>(b); }
	
	inline bignum operator+(const limb_t &b) const { bignum n=*this; return n += bignum<limb_bits>(b); }
	inline bignum operator-(const limb_t &b) const { bignum n=*this; return n -= bignum<limb_bits>(b); }
	inline bignum<limb_bits+BITCOUNT> operator*(const limb_t &b) const { bignum<limb_bits+BITCOUNT> n=this->trim<limb_bits+BITCOUNT>(); return n *= bignum<limb_bits>(b); }
	inline bignum operator/(const limb_t &b) const { bignum n=*this; return n /= bignum<limb_bits>(b); }
	inline bignum operator%(const limb_t &b) const { bignum n=*this; return n %= bignum<limb_bits>(b); }

	// Modulo, used by elliptic curve code:
	template <class bignumRet>
	inline bignumRet mod(const bignumRet &modby) const {
		return (*this % modby). template trim<bignumRet::NBIT>();
	}

	// Modular exponentiation, used throughout crypto 
	//  CAUTION: side channel attacks aplenty here!
	template <class bignumPow,class bignumRet>
	inline bignumRet powmod(bignumPow powby, const bignumRet &modby) const {
		bignumRet us=mod(modby);
		bignumRet ret=1;
		while (powby!=0) {
			if (powby.is_odd()) ret=(ret*us).mod(modby);
			powby=powby>>1; // go up to next bit of powby
			us=(us*us).mod(modby); // raise us to next power
		}
		return ret;
	}
	
	// Modular inverse wrapper:
	template <class bignumRet>
	inline bignumRet modInverse(const bignumRet &modby) const {
		bignumRet us=this->mod(modby);
		bignumRet modInv=bignumRet::knuth_modular_inverse(us,modby);
		
#if OBIGNUM_PARANOIA>=2 // validate that modular inverse is actually a modular inverse
		if ((modInv*us).mod(modby)!=1)
			obignum_die("modInverse isn't actually an inverse!",modInv,us);
#endif
		return modInv;
	}
	
	
	/**
	   Knuth's modular inverse algorithm with unsigned numbers.
	   	Computes u's inverse modulo v
	   
	   http://www.di-mgt.com.au/euclidean.html
	*/
	static bignum knuth_modular_inverse(const bignum &u,const bignum &v )
	{
		bignum u1=1, u3=u;
		bignum v1=0, v3=v;
		bool negative=false;
		while ( v3!=0 ) 
		{
			bignum quot, rem; // quotient, remainder
			
			divrem(quot,rem, u3,v3); // divide u3 / v3

			bignum t1=(u1+quot*v1). template trim<BITCOUNT>();
			
			/* swap down */
			u1=v1; v1=t1; u3=v3; v3=rem;
			negative=!negative;
		}
		
		if (u3!=1) obignum_die("No modular inverse exists!",u,v);
		
		bignum inv=u1;
		if (negative)  // result should be negative--modulo wraparound instead
			inv=(v-u1). template trim<BITCOUNT>();
		return inv;
	}
	
	/**
	   Extended euclidian greatest common denominator algorithm.
	   	Returns gcd(A,B) = A_mul*A + B_mul*B
	   A_mul and B_mul are called the "Bezout coefficients"
	   
	   http://en.wikipedia.org/wiki/Extended_Euclidean_algorithm
	*/
	static bignum extended_euclidean( bignum A, bignum B, bignum* A_mul, bignum* B_mul )
	{
		if (A<B) return extended_euclidean(B,A,B_mul,A_mul); // swap so A>=B
		
		bignum a0, b0, c0, a1, b1, c1, q, t;
		/* A        B      remainder */
		a0 = 1;  b0 = 0;  c0 = A;
		a1 = 0;  b1 = 1;  c1 = B;
		while ( c1!=0 ) 
		{
			// Do division
			q = c0/c1;

			t = a0;
			a0 = a1;
			a1 = (t - q*a1). template trim<BITCOUNT>();  // FAIL!  WILL GO NEGATIVE!
		
			t = b0;
			b0 = b1;
			b1 = (t - q*b1). template trim<BITCOUNT>();

			t = c0;
			c0 = c1;
			bignum qc1=(q*c1). template trim<BITCOUNT>();
#if OBIGNUM_PARANOIA>=2
			if (qc1>t) obignum_die("extended_euclidean went negative?!",qc1,t);
#endif
			c1 = (t - qc1). template trim<BITCOUNT>();
		}

		*A_mul = a0;
		*B_mul = b0;
		return c0;
	}

// Comparisons:
	template <int B_BITCOUNT,typename B_TRAITS> 
	inline int cmp(const bignum<B_BITCOUNT,B_TRAITS> &B) const // like strcmp, returns -1, 0, or +1
	{
		return cmp2(*this,B);
	}
	inline int cmp(const limb_t &b) const {
		return cmp(bignum<1>(b));
	}
	template <typename T> inline bool operator <(const T &b) const { return cmp(b) <0; }
	template <typename T> inline bool operator >(const T &b) const { return cmp(b) >0; }
	template <typename T> inline bool operator<=(const T &b) const { return cmp(b)<=0; }
	template <typename T> inline bool operator>=(const T &b) const { return cmp(b)>=0; }
	template <typename T> inline bool operator==(const T &b) const { return cmp(b)==0; }
	template <typename T> inline bool operator!=(const T &b) const { return cmp(b)!=0; }

// Printouts:
	void printLimb() const {
		for (int i=0;i<NLIMB;++i)
			std::cout<<"limb["<<i<<"]="<<std::setbase(16)<<limb[i]<<"\n";
		std::cout<<std::setbase(10);
	}
	void printHex(std::ostream &o=std::cout) const {
		o<<"0x"; 
		bool nonzero=false;
		for (int i=NLIMB-1;i>=0;--i) {
			if (limb[i]!=0) nonzero=true; // suppress leading zeros
			if (nonzero) 
				o<<
					std::setbase(16)<<std::setw(8)<<std::setfill('0')<< // I hate cout
					limb[i];
		}
		if (!nonzero) std::cout<<"0"; // but not all the zeros!
		o<<"\n"<<std::setbase(10);
	}
	friend std::ostream &operator<<(std::ostream &o,const bignum& b) {
		b.printHex(o);
		return o;
	}
};



/**
  The classic division interface:
     floor(numerator / denominator) = quotient
     numerator = quotient * denominator + remainder
  
  Simplest version just subtracts shifted copies of denominator.
  The number of copies is the quotient.  The leftover is the remainder.
  
  FIXME: Knuth division would be much more efficient (but side channel?)
*/
template <class Q,class R,class N,class D>
void divrem(Q &quotient,R &remainder,const N &numerator,const D &denom)
{
	//  Due to the implementation, temp values must have D+N bits.
	typedef bignum<N::NBIT+D::NBIT,typename N::TRAITS> T; 

	const static Q one(1);
	quotient=0; 
	T numleft=numerator.template trim<T::NBIT>(); // expand numerator
	int shiftCount;
	T shiftD; // == denominator << shiftCount
	for (shiftCount=N::NBIT-1;numleft>=denom && shiftCount>=0;shiftCount--)
	{
#if OBIGNUM_PARANOIA>=2 // sanity check shift count
		if (shiftCount<0) obignum_die("divrem shift count went negative!",numleft,denom);
#endif
		shiftLeft<T,D> shiftLoop(shiftD,denom,shiftCount);
//std::cout<<"  division "<<shiftCount<<": denomshifed="<<shiftD<<"   numleft="<<numleft<<"\n";
		if (shiftD<=numleft) { // this shift fits in the numerator
			//std::cout<<"   Division shift "<<shiftCount<<": subtracting denomshifted="<<shiftD<<" off of numleft="<<numleft<<"\n";
			sub3(numleft,numleft,shiftD);
			add3(quotient,quotient,one<<shiftCount); // add to quotient
			//std::cout<<"    new quotient="<<quotient<<"  new numleft="<<numleft<<"\n";
		}
	}
	remainder=numleft.template trim<R::NBIT>(); // leftover numerator == remainder
	
#if OBIGNUM_PARANOIA>=2 // check afterwards: quotient * denominator + remainder == numerator
	T check;
	mul(check,quotient,denom);
	add3(check,check,remainder);
	if (cmp2(check,numerator)!=0) {
		std::cout<<"Division self-test failure!\n";
		std::cout<<"  numerator="<<numerator<<"   denom="<<denom<<"\n";
		std::cout<<"  quotient="<<quotient<<"   remainder="<<remainder<<"\n";
		std::cout<<"  q*d+rem="<<check<<"\n";
		obignum_die("Failure in divrem self test",numerator,check);
	}
#endif
}


// Bitwise operations
#define MAKE_BITWISE_OP(op,classname, functionname) \
template <class D, class A, class B> \
class classname { \
	typename D::limb_t *dest; \
	const typename A::limb_t *Alimb; \
	const typename B::limb_t *Blimb; \
public: \
	inline classname(D &dest_, const A& a_, const B& b_) \
		:dest(dest_.limb), Alimb(a_.limb), Blimb(b_.limb) \
	{ \
		FORLOOP_SMARTUNROLL<classname,D::NLIMB,16>::run(*this); \
	} \
	inline void run(int i) { \
		dest[i] = Alimb[i] op Blimb[i]; \
	} \
}; \
template <class D, class A,class B> \
inline void functionname(D &d, const A& a, const B& b) \
	{ classname<D,A,B>(d,a,b); } \
\
template <int B_BITCOUNT,typename B_TRAITS> \
inline bignum<B_BITCOUNT,B_TRAITS> & \
operator op##=(bignum<B_BITCOUNT,B_TRAITS> &a, const bignum<B_BITCOUNT,B_TRAITS> &b) \
	{ functionname(a,a,b); return a; } \
\
template <int B_BITCOUNT,typename B_TRAITS> \
inline bignum<B_BITCOUNT,B_TRAITS> \
operator op(const bignum<B_BITCOUNT,B_TRAITS> &a, const bignum<B_BITCOUNT,B_TRAITS> &b) \
	{ bignum<B_BITCOUNT,B_TRAITS> ret; functionname(ret,a,b); return ret; }

MAKE_BITWISE_OP(&,andOpClass,and3);
MAKE_BITWISE_OP(|,orOpClass,or3);
MAKE_BITWISE_OP(^,xorOpClass,xor3);




/********** Elliptic Curve ************/

// Forward declaration of elliptic curve typedef
template <int BITCOUNT>
class ECcurve;

// One elliptic curve point, represented by 2D coordinates (x,y).
template <int BITCOUNT>
class ECpoint {
public:
	enum {NBIT=BITCOUNT};
	typedef bignum<NBIT> ECcoord;
	
	ECcoord x,y; // coordinates of point on the curve

	// Normal initialization
	ECpoint(const ECcoord &x_,const ECcoord &y_) :x(x_), y(y_) {}

	// By default, points are initialized to infinity.
	ECpoint() :x(infinity.x), y(infinity.y) {}

	// This is the "point at infinity".  It's arbitrarily far up.
	//   In the group of ECpoints, it's the identity element.
	static ECpoint infinity;

	friend std::ostream &operator<<(std::ostream &o,const ECpoint &p) {
		if (p == ECpoint::infinity) o<<"[infinity]";
		else o<<p.x<<" , "<<p.y;
		return o;
	}

	bool operator==(const ECpoint &p) const {
		return x==p.x && y==p.y;
	}
	bool operator!=(const ECpoint &p) const {
		return !operator==(p);
	}


	// Add us to o, under the action of this curve.
	ECpoint<BITCOUNT> add(const ECpoint<BITCOUNT> &o,const ECcurve<BITCOUNT> &curve) const;

	// Multiply us by this scalar, under the action of this curve.
	ECpoint<BITCOUNT> multiply(ECcoord scalar,const ECcurve<BITCOUNT> &curve) const;
};

// Initialize static infinity member
template <int BITCOUNT>
ECpoint<BITCOUNT>   ECpoint<BITCOUNT>::infinity(ECpoint<BITCOUNT>(
	typename ECpoint<BITCOUNT>::ECcoord(0)-1,
	typename ECpoint<BITCOUNT>::ECcoord(0)-1
));

/******************* curves *************************/
// Abstract superclass of modulo-a-prime type elliptic curves 
//  (I need to extend this to support binary GF(2^m) fields.)
template <int BITCOUNT>
class ECcurve {
public:
	enum {NBIT=BITCOUNT};
	typedef bignum<NBIT> ECcoord;
	typedef ECpoint<NBIT> point;
	
	// All curve arithmetic is modulo this huge prime:
	ECcoord p;

	// Here's an arbitrary starting point on the curve.
	//  This is called a curve generator G.
	ECpoint<NBIT> start;

	// This is the order of the start point
	ECcoord n;

	// Evaluate the elliptic curve at this point. 
	//  Returns 0 if the point lies along our curve.
	ECcoord evaluate_point(const point &p) const {
		if (p==point::infinity) return ECcoord(0); // infinity
		else return evaluate(p.x,p.y);
	}
	
	virtual ECcoord evaluate(const ECcoord &x,const ECcoord &y) const =0;

	// Evaluate the slope of the tangent of our curve at this curve point.
	virtual ECcoord tangent(const ECcoord &x,const ECcoord &y) const =0;
};

// The elliptic curve here is NIST p256r1: y^2 = x^3 - 3*x + b
//   http://csrc.nist.gov/groups/ST/toolkit/documents/dss/NISTReCur.pdf
//   See page 8, curve P-256.
class ECcurve_NISTP256 : public ECcurve<256> {
public:
	//    b is thus the constant part of the curve.
	ECcoord b;

	// Constructor sets initial values.
	ECcurve_NISTP256() {
		p.setHex("FFFFFFFF00000001000000000000000000000000FFFFFFFFFFFFFFFFFFFFFFFF");
		n.setHex("115792089210356248762697446949407573529996955224135760342422259061068512044369");
		b.setHex("5AC635D8AA3A93E7B3EBBD55769886BC651D06B0CC53B0F63BCE3C3E27D2604B");

		start.x.setHex("6B17D1F2E12C4247F8BCE6E563A440F277037D812DEB33A0F4A13945D898C296");
		start.y.setHex("4FE342E2FE1A7F9B8EE7EB4A7C0F9E162BCE33576B315ECECBB6406837BF51F5");
	}

	// Evaluate the elliptic curve at this point. 
	//  Returns 0 if the point lies along our curve.
	inline ECcoord evaluate(const ECcoord &x,const ECcoord &y) const {
		return ((x*x+(p-3)).mod(p)*x+b+p*p-y*y).mod(p);   // 0 = x^3 - 3*x + b - y^2
	}

	// Evaluate the slope of the tangent of our curve at this curve point.
	inline ECcoord tangent(const ECcoord &x,const ECcoord &y) const {
		return ((x*x*3+(p-3)).mod(p)*(y*2).modInverse(p)).mod(p); // == dy/dx
	}
};


// The elliptic curve here is SECG secp256k1: y^2 = x^3 + 7
//   See parameters in http://www.secg.org/collateral/sec2_final.pdf
//   section 2.7.1.  It's used by the BitCoin protocol.
class ECcurve_BitCoin : public ECcurve<256> {
public:
	// Constructor sets initial values.
	ECcurve_BitCoin() {
		p.setHex("FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFC2F");
		n.setHex("FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFE BAAEDCE6 AF48A03B BFD25E8C D0364141");

		// SEC's "04" means they're representing the generator point's X,Y parts explicitly.
		//  The compressed "02" form means storing only x (you compute Y)
		start.x.setHex("79BE667E F9DCBBAC 55A06295 CE870B07 029BFCDB 2DCE28D9 59F2815B 16F81798");
		start.y.setHex("483ADA77 26A3C465 5DA4FBFC 0E1108A8 FD17B448 A6855419 9C47D08F FB10D4B8");
	}

	// Evaluate the elliptic curve at this point. 
	//  Returns 0 if the point lies along our curve.
	inline ECcoord evaluate(const ECcoord &x,const ECcoord &y) const {
	
		//std::cout<<"Evaluating curve at x="<<x<<"  and y="<<y<<"\n";
		/*
		std::cout<<"   x*x*x = "<<x*x*x<<"\n";
		std::cout<<"   y*y = "<<y*y<<"\n";
		std::cout<<"   x*x*x+7 = "<<x*x*x+7<<"\n";
		std::cout<<"   x*x*x+7-y*y = "<<x*x*x+7-y*y<<"\n";
		std::cout<<"   (x*x*x+7-y*y).mod(p) = "<<(x*x*x+7-y*y).mod(p)<<"\n";
		*/
	
		return ((x*x).mod(p)*x+7+p*p-y*y).mod(p);   // 0 = x^3 +7 - y^2
	}

	// Evaluate the slope of the tangent of our curve at this curve point.
	inline ECcoord tangent(const ECcoord &x,const ECcoord &y) const {
		return ((x*x*3).mod(p)*(y*2).modInverse(p)).mod(p); // == dy/dx
	}
};

// General form for all brainpool or secg r curves: y^2 = x^3 + Ax + B mod p
template <int BITCOUNT>
class ECcurve_AB : public ECcurve<BITCOUNT> {
public:	
	typedef ECcurve<BITCOUNT> parent;
	typedef typename parent::ECcoord ECcoord;
	using parent::p;
	
	ECcoord A,B;
	
	inline ECcoord evaluate(const ECcoord &x,const ECcoord &y) const {
		return ((x*x+A).mod(p)*x+B+p*p-y*y).mod(p);   // 0 = x^3 +Ax+B - y^2
	}

	// Evaluate the slope of the tangent of our curve at this curve point.
	inline ECcoord tangent(const ECcoord &x,const ECcoord &y) const {
		return ((x*x*3+A).mod(p)*(y*2).modInverse(p)).mod(p); // == dy/dx
	}
};

// The elliptic curve here is SECG secp521r1: y^2 = x^3 + 7
//   See parameters in http://www.secg.org/collateral/sec2_final.pdf
class ECcurve_secp521r1 : public ECcurve_AB<521> {
public: 
	typedef typename ECcurve_AB<521>::ECcoord ECcoord;
	
	// Constructor sets initial values.
	ECcurve_secp521r1() {
		p.setHex("01FF FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF"
		"FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF"
		"FFFFFFFF FFFFFFFF FFFFFFFF");
		n.setHex("01FF FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF"
		"FFFFFFFF FFFFFFFA 51868783 BF2F966B 7FCC0148 F709A5D0 3BB5C9B8"
		"899C47AE BB6FB71E 91386409");

		A.setHex("01FF FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF"
		"FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF"
		"FFFFFFFF FFFFFFFF FFFFFFFC");
		B.setHex("0051 953EB961 8E1C9A1F 929A21A0 B68540EE A2DA725B 99B315F3"
		"B8B48991 8EF109E1 56193951 EC7E937B 1652C0BD 3BB1BF07 3573DF88"
		"3D2C34F1 EF451FD4 6B503F00");

		// SEC's "04" means they're representing the generator point's X,Y parts explicitly.
		//  The compressed "02" form means storing only x (you compute Y)
		start.x.setHex("C6858E 06B70404 E9CD9E3E CB662395 B4429C64 8139053F"
		"B521F828 AF606B4D 3DBAA14B 5E77EFE7 5928FE1D C127A2FF A8DE3348"
		"B3C1856A 429BF97E 7E31C2E5 BD66");
		start.y.setHex("0118 39296A78 9A3BC004 5C8A5FB4"
		"2C7D1BD9 98F54449 579B4468 17AFBD17 273E662C 97EE7299 5EF42640"
		"C550B901 3FAD0761 353C7086 A272C240 88BE9476 9FD16650");
	}
};

// Brainpool 512-bit curve: 
class ECcurve_brainpoolP512r1 : public ECcurve_AB<512> {
public:
	typedef typename ECcurve_AB<512>::ECcoord ECcoord;
	
	ECcurve_brainpoolP512r1() {
		p.setHex("AADD9DB8DBE9C48B3FD4E6AE33C9FC07CB308DB3B3C9D20ED6639CCA703308717D4D9B009BC66842AECDA12AE6A380E62881FF2F2D82C68528AA6056583A48F3");
		n.setHex("AADD9DB8DBE9C48B3FD4E6AE33C9FC07CB308DB3B3C9D20ED6639CCA70330870553E5C414CA92619418661197FAC10471DB1D381085DDADDB58796829CA90069");
		start.x.setHex("81AEE4BDD82ED9645A21322E9C4C6A9385ED9F70B5D916C1B43B62EEF4D0098EFF3B1F78E2D0D48D50D1687B93B97D5F7C6D5047406A5E688B352209BCB9F822");
		start.y.setHex("7DDE385D566332ECC0EABFA9CF7822FDF209F70024A57B1AA000C55B881F8111B2DCDE494A5F485E5BCA4BD88A2763AED1CA2B2FA8F0540678CD1E0F3AD80892");
		
		A.setHex("7830A3318B603B89E2327145AC234CC594CBDD8D3DF91610A83441CAEA9863BC2DED5D5AA8253AA10A2EF1C98B9AC8B57F1117A72BF2C7B9E7C1AC4D77FC94CA");
		B.setHex ("3DF91610A83441CAEA9863BC2DED5D5AA8253AA10A2EF1C98B9AC8B57F1117A72BF2C7B9E7C1AC4D77FC94CADC083E67984050B75EBAE5DD2809BD638016F723");	
	}
};


/*** Details of elliptic curve - point multiplication ********/
template <int BITCOUNT>
ECpoint<BITCOUNT> 
ECpoint<BITCOUNT>::add(const ECpoint<BITCOUNT> &o,const ECcurve<BITCOUNT> &curve) const
{
	// special cases for infinity:
	if (*this == infinity) return o; /* we're zero */
	if (o == infinity) return *this; /* they're zero */

	// Usual case:
	const ECcoord &p=curve.p;
	ECcoord m;
	if (x==o.x) { // special case for same X coordinate
		if (y==o.y) { // adding point to itself--take tangent
			m=curve.tangent(x,y);
		} else { // adding additive inverses--return "infinity"
			return infinity;
		}
	}
	else { // default case: 
		m=((y+(p-o.y)) * (x+(p-o.x)).modInverse(p)).mod(p); // dy/dx slope of line (default case)
	}
//std::cout<<"EC slope m="<<m<<"\n";

	ECcoord x3 = (m*m + (p - x) + (p - o.x)).mod(p); // comes from matching x^2 terms
	
	// line's y intercept  v = y - m*x
	//  point's Y coordinate = -(m*x3 + v)
	
	ECcoord y3 = (m*x+(p-y)+p*p-m*x3).mod(p); // from line equation, plus mirroring

	return ECpoint<NBIT>(x3,y3);
}

// Multiply us by this scalar, under the action of this curve.
//   FIXME: side channel attacks aplenty here!
template <int BITCOUNT>
ECpoint<BITCOUNT> 
ECpoint<BITCOUNT>::multiply(typename ECpoint<BITCOUNT>::ECcoord scalar,
	const ECcurve<BITCOUNT> &curve) const
{
	ECpoint<NBIT> sum=ECpoint<NBIT>::infinity; 
	ECpoint<NBIT> A=*this;
	for (int bit=0;scalar!=0;bit++) {
		if ( scalar.is_odd() ) // this bit is set--include value in sum
		{ // do sum+=A;
			//std::cout<<"ECpoint::multiply odd "<<scalar<<" -> Adding A="<<A<<"\n";
			sum=sum.add(A,curve);
		}
		scalar = scalar>>1; // shift right by 1 bit
		if (scalar!=0) { // more data is available
			A=A.add(A,curve); // shift A up to next bit (A+=A)
			//std::cout<<"ECpoint::multiply new A="<<A<<"\n";
		}
		// possible optimization: save the table of A values...
	}
	return sum;
}

#endif /* defined(thisHeader) */

