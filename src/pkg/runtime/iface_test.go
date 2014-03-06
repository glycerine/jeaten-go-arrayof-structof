// Copyright 2012 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

package runtime_test

import (
	. "runtime"
	"testing"
	"unsafe"
)

type I1 interface {
	Method1()
}

type I2 interface {
	Method1()
	Method2()
}

type TS uint16
type TM uintptr
type TL [2]uintptr

func (TS) Method1() {}
func (TS) Method2() {}
func (TM) Method1() {}
func (TM) Method2() {}
func (TL) Method1() {}
func (TL) Method2() {}

var (
	e  interface{}
	e_ interface{}
	i1 I1
	i2 I2
	ts TS
	tm TM
	tl TL
)

func BenchmarkConvT2ESmall(b *testing.B) {
	for i := 0; i < b.N; i++ {
		e = ts
	}
}

func BenchmarkConvT2EUintptr(b *testing.B) {
	for i := 0; i < b.N; i++ {
		e = tm
	}
}

func BenchmarkConvT2ELarge(b *testing.B) {
	for i := 0; i < b.N; i++ {
		e = tl
	}
}

func BenchmarkConvT2ISmall(b *testing.B) {
	for i := 0; i < b.N; i++ {
		i1 = ts
	}
}

func BenchmarkConvT2IUintptr(b *testing.B) {
	for i := 0; i < b.N; i++ {
		i1 = tm
	}
}

func BenchmarkConvT2ILarge(b *testing.B) {
	for i := 0; i < b.N; i++ {
		i1 = tl
	}
}

func BenchmarkConvI2E(b *testing.B) {
	i2 = tm
	for i := 0; i < b.N; i++ {
		e = i2
	}
}

func BenchmarkConvI2I(b *testing.B) {
	i2 = tm
	for i := 0; i < b.N; i++ {
		i1 = i2
	}
}

func BenchmarkAssertE2T(b *testing.B) {
	e = tm
	for i := 0; i < b.N; i++ {
		tm = e.(TM)
	}
}

func BenchmarkAssertE2TLarge(b *testing.B) {
	e = tl
	for i := 0; i < b.N; i++ {
		tl = e.(TL)
	}
}

func BenchmarkAssertE2I(b *testing.B) {
	e = tm
	for i := 0; i < b.N; i++ {
		i1 = e.(I1)
	}
}

func BenchmarkAssertI2T(b *testing.B) {
	i1 = tm
	for i := 0; i < b.N; i++ {
		tm = i1.(TM)
	}
}

func BenchmarkAssertI2I(b *testing.B) {
	i1 = tm
	for i := 0; i < b.N; i++ {
		i2 = i1.(I2)
	}
}

func BenchmarkAssertI2E(b *testing.B) {
	i1 = tm
	for i := 0; i < b.N; i++ {
		e = i1.(interface{})
	}
}

func BenchmarkAssertE2E(b *testing.B) {
	e = tm
	for i := 0; i < b.N; i++ {
		e_ = e
	}
}

func TestGenericEq(t *testing.T) {
	testGenericAlg(t, GenericEq, "equal")
}

func TestGenericHash(t *testing.T) {
	test := func(px, py, pt unsafe.Pointer) bool {
		return GenericHash(px, pt) == GenericHash(py, pt)
	}
	testGenericAlg(t, test, "hash equal")
}

func testGenericAlg(t *testing.T, test func(px, py, pt unsafe.Pointer) bool, op string) {
	var (
		// Equal strings that stored at different locations in memory
		s1 = "abc"
		s2 = (s1 + "d")[:3]

		zero uintptr = 0
		ones uintptr = zero - 1
	)

	type S struct {
		_ byte
		a string
		b uint32
	}

	// Note that because S is compiled into the binary, it has its own alg table.
	// Tests for type A will not recursively use GenericAlgX for type S.
	// TODO(crc) Update comment to reference more comprehensive equal/hash tests in
	// reflect once ArrayOf/StructOf patch is accepted.
	type A [2]S

	x, y := A{}, A{}
	px, py := unsafe.Pointer(&x), unsafe.Pointer(&y)
	ta := typeOf(x)
	ts := typeOf(S{})

	// Test the two zero'd values are equal
	if !test(px, py, ta) {
		t.Errorf("Zero'd arrays %v %v are not %s.", x, y, op)
	}
	if !test(px, py, ts) {
		t.Errorf("Zero'd structs %v %v are not %s.", x[0], y[0], op)
	}

	// Probe ones into the padding between _ and a
	*(*uintptr)(px) = ones
	*(*byte)(px) = 0
	if !test(px, py, ta) {
		t.Errorf("padding compared making arrays %v %v not %s.", x, y, op)
	}
	if !test(px, py, ts) {
		t.Errorf("padding compared making structs %v %v not %s.", x[0], y[0], op)
	}

	// Probe ones into _
	*(*uintptr)(px) = zero
	*(*byte)(px) = byte(ones)
	if !test(px, py, ta) {
		t.Errorf("_ compared making arrays %v %v not %s.", x, y, op)
	}
	if !test(px, py, ts) {
		t.Errorf("_ compared making structs %v %v not %s.", x[0], y[0], op)
	}

	// Test for equality
	x, y = A{{a: s1}}, A{{a: s2}}
	if !test(px, py, ta) {
		t.Errorf("arrays %v %v should be %s.", x, y, op)
	}
	if !test(px, py, ts) {
		t.Errorf("structs %v %v should be %s.", x[0], y[0], op)
	}

	// Test for equality
	x, y = A{{a: "abc"}}, A{{a: "def"}}
	if test(px, py, ta) {
		t.Errorf("arrays %v %v should not be %s.", x, y, op)
	}
	if test(px, py, ts) {
		t.Errorf("structs %v %v should not be %s.", x[0], y[0], op)
	}

	// Run test on a somewhat complicated struct to weed out corner cases
	type Complicated struct {
		_ byte
		a int16
		b uint32
		c complex64
		d byte
		e complex128
		f string
		g chan int
		h interface{}
		i A
		j S
		k unsafe.Pointer
	}

	complicated := Complicated{
		g: make(chan int),
		h: 1,
	}
	pc := unsafe.Pointer(&complicated)
	complicated.k = pc
	tc := typeOf(complicated)
	if !test(pc, pc, tc) {
		t.Errorf("complicated struct not %v to itself.", op)
	}

	// Test uncomparable types
	func() {
		type Uncomparable struct {
			_ []int
		}
		uncomparable := Uncomparable{}
		pu := unsafe.Pointer(&uncomparable)
		tu := typeOf(uncomparable)
		defer func() {
			if e := recover(); e == nil {
				t.Errorf("uncomparable struct did not panic on '%v'.", op)
			}
		}()
		test(pu, pu, tu)
	}()

	// Test panic on types where generic algorithms are not required
	func() {
		pr := unsafe.Pointer(&s1)
		tr := typeOf(s1)
		defer func() {
			if e := recover(); e == nil {
				t.Errorf("type not requiring generic ald did not panic on '%v'.", op)
			}
		}()
		test(pr, pr, tr)
	}()

}

func typeOf(i interface{}) unsafe.Pointer {
	return *((*unsafe.Pointer)(unsafe.Pointer(&i)))
}
