// Copyright 2009 The Go Authors.  All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

package eval

import "os"

/*
 * Virtual machine
 */

type Thread struct {
	abort chan os.Error
	pc    uint
	// The execution frame of this function.  This remains the
	// same throughout a function invocation.
	f *Frame
}

type code []func(*Thread)

func (i code) exec(t *Thread) {
	opc := t.pc
	t.pc = 0
	l := uint(len(i))
	for t.pc < l {
		pc := t.pc
		t.pc++
		i[pc](t)
	}
	t.pc = opc
}

/*
 * Code buffer
 */

type codeBuf struct {
	instrs code
}

func newCodeBuf() *codeBuf { return &codeBuf{make(code, 0, 16)} }

func (b *codeBuf) push(instr func(*Thread)) {
	n := len(b.instrs)
	if n >= cap(b.instrs) {
		a := make(code, n, n*2)
		copy(a, b.instrs)
		b.instrs = a
	}
	b.instrs = b.instrs[0 : n+1]
	b.instrs[n] = instr
}

func (b *codeBuf) nextPC() uint { return uint(len(b.instrs)) }

func (b *codeBuf) get() code {
	// Freeze this buffer into an array of exactly the right size
	a := make(code, len(b.instrs))
	copy(a, b.instrs)
	return code(a)
}

/*
 * User-defined functions
 */

type evalFunc struct {
	outer     *Frame
	frameSize int
	code      code
}

func (f *evalFunc) NewFrame() *Frame { return f.outer.child(f.frameSize) }

func (f *evalFunc) Call(t *Thread) { f.code.exec(t) }
