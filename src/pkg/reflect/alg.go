package reflect

import (
	"unsafe"
)

type algTable struct {
	hash unsafe.Pointer
	equal unsafe.Pointer
	print unsafe.Pointer
	copy unsafe.Pointer
}

// NOTE: These are copied from ../runtime/runtime.h.
// They must be kept in sync.
const (
	_AMEM = iota
	_AMEM0
	_AMEM8
	_AMEM16
	_AMEM32
	_AMEM64
	_AMEM128
	_ANOEQ
	_ANOEQ0
	_ANOEQ8
	_ANOEQ16
	_ANOEQ32
	_ANOEQ64
	_ANOEQ128
	_ASTRING
	_AINTER
	_ANILINTER
	_ASLICE
	_AFLOAT32
	_AFLOAT64
	_ACPLX64
	_ACPLX128
	_Amax
)
func alg(i int) *algTable

// Is only needed by arrays and structs. Generic hash|equal alg panics for anything else.
func algForType(t *rtype, comparable, continuous bool) *algTable {
	var i int
	switch t.size {
	case 0:
		i = _AMEM0
	case 8:
		i = _AMEM8
	case 16:
		i = _AMEM16
	case 32:
		i = _AMEM32
	case 64:
		i = _AMEM64
	case 128:
		i = _AMEM128
	default:
		i = _AMEM
	}

	if !comparable {
		if i == _AMEM {
			i = _ANOEQ
		} else {
			i += _ANOEQ0 - _AMEM0
		}
	}

	a := alg(i)

	if comparable && !continuous {
		// Create an algTable specially for this type. nil implies use generic algs.
		aa := *a
		aa.equal = nil
		aa.hash = nil
		a = &aa
	}
	return a
}

var (
	noequal = TypeOf([]int{}).(*rtype).alg.equal
	amem = uintptr(unsafe.Pointer(alg(_AMEM)))
	astring = uintptr(unsafe.Pointer(alg(_ASTRING)))
	ainter = uintptr(unsafe.Pointer(alg(_AINTER)))
	anilinter = uintptr(unsafe.Pointer(alg(_ANILINTER)))
	aslice = uintptr(unsafe.Pointer(alg(_ASLICE)))
	amax = uintptr(unsafe.Pointer(alg(_Amax)))
)

// Check if the rtype.alg field has a non-panicing compare algorithm
func comparable(t *rtype) bool {
	return t.alg.equal != noequal
}

// Check if the rtype.alg field compares only memory.
func memalg(t *rtype) bool {
	a := uintptr(unsafe.Pointer(t.alg))
	return !(a == astring || a == ainter || a == anilinter || a == aslice ||
		(a < amem || a >= amax)) // implementations outside algarray
}
