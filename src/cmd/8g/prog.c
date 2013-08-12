// Copyright 2013 The Go Authors.  All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include <u.h>
#include <libc.h>
#include "gg.h"
#include "opt.h"

// Matches real RtoB but can be used in global initializer.
#define RtoB(r) (1<<((r)-D_AX))

enum {
	AX = RtoB(D_AX),
	BX = RtoB(D_BX),
	CX = RtoB(D_CX),
	DX = RtoB(D_DX),
	DI = RtoB(D_DI),
	SI = RtoB(D_SI),
	
	LeftRdwr = LeftRead | LeftWrite,
	RightRdwr = RightRead | RightWrite,
};

#undef RtoB

// This table gives the basic information about instruction
// generated by the compiler and processed in the optimizer.
// See opt.h for bit definitions.
//
// Instructions not generated need not be listed.
// As an exception to that rule, we typically write down all the
// size variants of an operation even if we just use a subset.
//
// The table is formatted for 8-space tabs.
static ProgInfo progtable[ALAST] = {
	[ATYPE]=	{Pseudo | Skip},
	[ATEXT]=	{Pseudo},
	[AFUNCDATA]=	{Pseudo},
	[APCDATA]=	{Pseudo},
	[AUNDEF]=	{OK},
	[AUSEFIELD]=	{OK},

	// NOP is an internal no-op that also stands
	// for USED and SET annotations, not the Intel opcode.
	[ANOP]=		{LeftRead | RightWrite},

	[AADCL]=	{SizeL | LeftRead | RightRdwr | SetCarry | UseCarry},
	[AADCW]=	{SizeW | LeftRead | RightRdwr | SetCarry | UseCarry},

	[AADDB]=	{SizeB | LeftRead | RightRdwr | SetCarry},
	[AADDL]=	{SizeL | LeftRead | RightRdwr | SetCarry},
	[AADDW]=	{SizeW | LeftRead | RightRdwr | SetCarry},
	
	[AADDSD]=	{SizeD | LeftRead | RightRdwr},
	[AADDSS]=	{SizeF | LeftRead | RightRdwr},

	[AANDB]=	{SizeB | LeftRead | RightRdwr | SetCarry},
	[AANDL]=	{SizeL | LeftRead | RightRdwr | SetCarry},
	[AANDW]=	{SizeW | LeftRead | RightRdwr | SetCarry},

	[ACALL]=	{RightAddr | Call | KillCarry},

	[ACDQ]=		{OK, AX, AX | DX},
	[ACWD]=		{OK, AX, AX | DX},

	[ACLD]=		{OK},
	[ASTD]=		{OK},

	[ACMPB]=	{SizeB | LeftRead | RightRead | SetCarry},
	[ACMPL]=	{SizeL | LeftRead | RightRead | SetCarry},
	[ACMPW]=	{SizeW | LeftRead | RightRead | SetCarry},

	[ACOMISD]=	{SizeD | LeftRead | RightRead | SetCarry},
	[ACOMISS]=	{SizeF | LeftRead | RightRead | SetCarry},

	[ACVTSD2SL]=	{SizeL | LeftRead | RightWrite | Conv},
	[ACVTSD2SS]=	{SizeF | LeftRead | RightWrite | Conv},
	[ACVTSL2SD]=	{SizeD | LeftRead | RightWrite | Conv},
	[ACVTSL2SS]=	{SizeF | LeftRead | RightWrite | Conv},
	[ACVTSS2SD]=	{SizeD | LeftRead | RightWrite | Conv},
	[ACVTSS2SL]=	{SizeL | LeftRead | RightWrite | Conv},
	[ACVTTSD2SL]=	{SizeL | LeftRead | RightWrite | Conv},
	[ACVTTSS2SL]=	{SizeL | LeftRead | RightWrite | Conv},

	[ADECB]=	{SizeB | RightRdwr},
	[ADECL]=	{SizeL | RightRdwr},
	[ADECW]=	{SizeW | RightRdwr},

	[ADIVB]=	{SizeB | LeftRead, AX, AX},
	[ADIVL]=	{SizeL | LeftRead, AX|DX, AX|DX},
	[ADIVW]=	{SizeW | LeftRead, AX|DX, AX|DX},

	[ADIVSD]=	{SizeD | LeftRead | RightRdwr},
	[ADIVSS]=	{SizeF | LeftRead | RightRdwr},
	
	[AFLDCW]=	{SizeW | LeftAddr },
	[AFSTCW]=	{SizeW | RightAddr },

	[AFADDD]=	{SizeD | LeftAddr | RightRdwr},
	[AFADDDP]=	{SizeD | LeftAddr | RightRdwr},
	[AFADDF]=	{SizeF | LeftAddr | RightRdwr},

	[AFCOMD]=	{SizeD | LeftAddr | RightRead},
	[AFCOMDP]=	{SizeD | LeftAddr | RightRead},
	[AFCOMDPP]=	{SizeD | LeftAddr | RightRead},
	[AFCOMF]=	{SizeF | LeftAddr | RightRead},
	[AFCOMFP]=	{SizeF | LeftAddr | RightRead},
	[AFUCOMIP]=	{SizeF | LeftAddr | RightRead},

	[AFCHS]=	{SizeD | RightRdwr}, // also SizeF

	[AFDIVD]=	{SizeD | LeftAddr | RightRdwr},
	[AFDIVDP]=	{SizeD | LeftAddr | RightRdwr},
	[AFDIVF]=	{SizeF | LeftAddr | RightRdwr},

	[AFSUBD]=	{SizeD | LeftAddr | RightRdwr},
	[AFSUBDP]=	{SizeD | LeftAddr | RightRdwr},
	[AFSUBF]=	{SizeF | LeftAddr | RightRdwr},
	[AFSUBRD]=	{SizeD | LeftAddr | RightRdwr},
	[AFSUBRDP]=	{SizeD | LeftAddr | RightRdwr},
	[AFSUBRF]=	{SizeF | LeftAddr | RightRdwr},

	[AFMOVD]=	{SizeD | LeftAddr | RightWrite},
	[AFMOVF]=	{SizeF | LeftAddr | RightWrite},
	[AFMOVL]=	{SizeL | LeftAddr | RightWrite},
	[AFMOVW]=	{SizeW | LeftAddr | RightWrite},
	[AFMOVV]=	{SizeQ | LeftAddr | RightWrite},

	[AFMOVDP]=	{SizeD | LeftRead | RightAddr},
	[AFMOVFP]=	{SizeF | LeftRead | RightAddr},
	[AFMOVLP]=	{SizeL | LeftRead | RightAddr},
	[AFMOVWP]=	{SizeW | LeftRead | RightAddr},
	[AFMOVVP]=	{SizeQ | LeftRead | RightAddr},

	[AFMULD]=	{SizeD | LeftAddr | RightRdwr},
	[AFMULDP]=	{SizeD | LeftAddr | RightRdwr},
	[AFMULF]=	{SizeF | LeftAddr | RightRdwr},

	[AIDIVB]=	{SizeB | LeftRead, AX, AX},
	[AIDIVL]=	{SizeL | LeftRead, AX|DX, AX|DX},
	[AIDIVW]=	{SizeW | LeftRead, AX|DX, AX|DX},

	[AIMULB]=	{SizeB | LeftRead | SetCarry, AX, AX},
	[AIMULL]=	{SizeL | LeftRead | ImulAXDX | SetCarry},
	[AIMULW]=	{SizeW | LeftRead | ImulAXDX | SetCarry},

	[AINCB]=	{SizeB | RightRdwr},
	[AINCL]=	{SizeL | RightRdwr},
	[AINCW]=	{SizeW | RightRdwr},

	[AJCC]=		{Cjmp | UseCarry},
	[AJCS]=		{Cjmp | UseCarry},
	[AJEQ]=		{Cjmp | UseCarry},
	[AJGE]=		{Cjmp | UseCarry},
	[AJGT]=		{Cjmp | UseCarry},
	[AJHI]=		{Cjmp | UseCarry},
	[AJLE]=		{Cjmp | UseCarry},
	[AJLS]=		{Cjmp | UseCarry},
	[AJLT]=		{Cjmp | UseCarry},
	[AJMI]=		{Cjmp | UseCarry},
	[AJNE]=		{Cjmp | UseCarry},
	[AJOC]=		{Cjmp | UseCarry},
	[AJOS]=		{Cjmp | UseCarry},
	[AJPC]=		{Cjmp | UseCarry},
	[AJPL]=		{Cjmp | UseCarry},
	[AJPS]=		{Cjmp | UseCarry},

	[AJMP]=		{Jump | Break | KillCarry},

	[ALEAL]=	{LeftAddr | RightWrite},

	[AMOVBLSX]=	{SizeL | LeftRead | RightWrite | Conv},
	[AMOVBLZX]=	{SizeL | LeftRead | RightWrite | Conv},
	[AMOVBWSX]=	{SizeW | LeftRead | RightWrite | Conv},
	[AMOVBWZX]=	{SizeW | LeftRead | RightWrite | Conv},
	[AMOVWLSX]=	{SizeL | LeftRead | RightWrite | Conv},
	[AMOVWLZX]=	{SizeL | LeftRead | RightWrite | Conv},

	[AMOVB]=	{SizeB | LeftRead | RightWrite | Move},
	[AMOVL]=	{SizeL | LeftRead | RightWrite | Move},
	[AMOVW]=	{SizeW | LeftRead | RightWrite | Move},

	[AMOVSB]=	{OK, DI|SI, DI|SI},
	[AMOVSL]=	{OK, DI|SI, DI|SI},
	[AMOVSW]=	{OK, DI|SI, DI|SI},

	[AMOVSD]=	{SizeD | LeftRead | RightWrite | Move},
	[AMOVSS]=	{SizeF | LeftRead | RightWrite | Move},

	// We use MOVAPD as a faster synonym for MOVSD.
	[AMOVAPD]=	{SizeD | LeftRead | RightWrite | Move},

	[AMULB]=	{SizeB | LeftRead | SetCarry, AX, AX},
	[AMULL]=	{SizeL | LeftRead | SetCarry, AX, AX|DX},
	[AMULW]=	{SizeW | LeftRead | SetCarry, AX, AX|DX},
	
	[AMULSD]=	{SizeD | LeftRead | RightRdwr},
	[AMULSS]=	{SizeF | LeftRead | RightRdwr},

	[ANEGB]=	{SizeB | RightRdwr | SetCarry},
	[ANEGL]=	{SizeL | RightRdwr | SetCarry},
	[ANEGW]=	{SizeW | RightRdwr | SetCarry},

	[ANOTB]=	{SizeB | RightRdwr},
	[ANOTL]=	{SizeL | RightRdwr},
	[ANOTW]=	{SizeW | RightRdwr},

	[AORB]=		{SizeB | LeftRead | RightRdwr | SetCarry},
	[AORL]=		{SizeL | LeftRead | RightRdwr | SetCarry},
	[AORW]=		{SizeW | LeftRead | RightRdwr | SetCarry},

	[APOPL]=	{SizeL | RightWrite},
	[APUSHL]=	{SizeL | LeftRead},

	[ARCLB]=	{SizeB | LeftRead | RightRdwr | ShiftCX | SetCarry | UseCarry},
	[ARCLL]=	{SizeL | LeftRead | RightRdwr | ShiftCX | SetCarry | UseCarry},
	[ARCLW]=	{SizeW | LeftRead | RightRdwr | ShiftCX | SetCarry | UseCarry},

	[ARCRB]=	{SizeB | LeftRead | RightRdwr | ShiftCX | SetCarry | UseCarry},
	[ARCRL]=	{SizeL | LeftRead | RightRdwr | ShiftCX | SetCarry | UseCarry},
	[ARCRW]=	{SizeW | LeftRead | RightRdwr | ShiftCX | SetCarry | UseCarry},

	[AREP]=		{OK, CX, CX},
	[AREPN]=	{OK, CX, CX},

	[ARET]=		{Break | KillCarry},

	[AROLB]=	{SizeB | LeftRead | RightRdwr | ShiftCX | SetCarry},
	[AROLL]=	{SizeL | LeftRead | RightRdwr | ShiftCX | SetCarry},
	[AROLW]=	{SizeW | LeftRead | RightRdwr | ShiftCX | SetCarry},

	[ARORB]=	{SizeB | LeftRead | RightRdwr | ShiftCX | SetCarry},
	[ARORL]=	{SizeL | LeftRead | RightRdwr | ShiftCX | SetCarry},
	[ARORW]=	{SizeW | LeftRead | RightRdwr | ShiftCX | SetCarry},

	[ASALB]=	{SizeB | LeftRead | RightRdwr | ShiftCX | SetCarry},
	[ASALL]=	{SizeL | LeftRead | RightRdwr | ShiftCX | SetCarry},
	[ASALW]=	{SizeW | LeftRead | RightRdwr | ShiftCX | SetCarry},

	[ASARB]=	{SizeB | LeftRead | RightRdwr | ShiftCX | SetCarry},
	[ASARL]=	{SizeL | LeftRead | RightRdwr | ShiftCX | SetCarry},
	[ASARW]=	{SizeW | LeftRead | RightRdwr | ShiftCX | SetCarry},

	[ASBBB]=	{SizeB | LeftRead | RightRdwr | SetCarry | UseCarry},
	[ASBBL]=	{SizeL | LeftRead | RightRdwr | SetCarry | UseCarry},
	[ASBBW]=	{SizeW | LeftRead | RightRdwr | SetCarry | UseCarry},

	[ASETCC]=	{SizeB | RightRdwr | UseCarry},
	[ASETCS]=	{SizeB | RightRdwr | UseCarry},
	[ASETEQ]=	{SizeB | RightRdwr | UseCarry},
	[ASETGE]=	{SizeB | RightRdwr | UseCarry},
	[ASETGT]=	{SizeB | RightRdwr | UseCarry},
	[ASETHI]=	{SizeB | RightRdwr | UseCarry},
	[ASETLE]=	{SizeB | RightRdwr | UseCarry},
	[ASETLS]=	{SizeB | RightRdwr | UseCarry},
	[ASETLT]=	{SizeB | RightRdwr | UseCarry},
	[ASETMI]=	{SizeB | RightRdwr | UseCarry},
	[ASETNE]=	{SizeB | RightRdwr | UseCarry},
	[ASETOC]=	{SizeB | RightRdwr | UseCarry},
	[ASETOS]=	{SizeB | RightRdwr | UseCarry},
	[ASETPC]=	{SizeB | RightRdwr | UseCarry},
	[ASETPL]=	{SizeB | RightRdwr | UseCarry},
	[ASETPS]=	{SizeB | RightRdwr | UseCarry},

	[ASHLB]=	{SizeB | LeftRead | RightRdwr | ShiftCX | SetCarry},
	[ASHLL]=	{SizeL | LeftRead | RightRdwr | ShiftCX | SetCarry},
	[ASHLW]=	{SizeW | LeftRead | RightRdwr | ShiftCX | SetCarry},

	[ASHRB]=	{SizeB | LeftRead | RightRdwr | ShiftCX | SetCarry},
	[ASHRL]=	{SizeL | LeftRead | RightRdwr | ShiftCX | SetCarry},
	[ASHRW]=	{SizeW | LeftRead | RightRdwr | ShiftCX | SetCarry},

	[ASTOSB]=	{OK, AX|DI, DI},
	[ASTOSL]=	{OK, AX|DI, DI},
	[ASTOSW]=	{OK, AX|DI, DI},

	[ASUBB]=	{SizeB | LeftRead | RightRdwr | SetCarry},
	[ASUBL]=	{SizeL | LeftRead | RightRdwr | SetCarry},
	[ASUBW]=	{SizeW | LeftRead | RightRdwr | SetCarry},

	[ASUBSD]=	{SizeD | LeftRead | RightRdwr},
	[ASUBSS]=	{SizeF | LeftRead | RightRdwr},

	[ATESTB]=	{SizeB | LeftRead | RightRead},
	[ATESTL]=	{SizeL | LeftRead | RightRead},
	[ATESTW]=	{SizeW | LeftRead | RightRead},

	[AUCOMISD]=	{SizeD | LeftRead | RightRead},
	[AUCOMISS]=	{SizeF | LeftRead | RightRead},

	[AXCHGB]=	{SizeB | LeftRdwr | RightRdwr},
	[AXCHGL]=	{SizeL | LeftRdwr | RightRdwr},
	[AXCHGW]=	{SizeW | LeftRdwr | RightRdwr},

	[AXORB]=	{SizeB | LeftRead | RightRdwr | SetCarry},
	[AXORL]=	{SizeL | LeftRead | RightRdwr | SetCarry},
	[AXORW]=	{SizeW | LeftRead | RightRdwr | SetCarry},
};

void
proginfo(ProgInfo *info, Prog *p)
{
	*info = progtable[p->as];
	if(info->flags == 0)
		fatal("unknown instruction %P", p);

	if((info->flags & ShiftCX) && p->from.type != D_CONST)
		info->reguse |= CX;

	if(info->flags & ImulAXDX) {
		if(p->to.type == D_NONE) {
			info->reguse |= AX;
			info->regset |= AX | DX;
		} else {
			info->flags |= RightRdwr;
		}
	}

	// Addressing makes some registers used.
	if(p->from.type >= D_INDIR)
		info->regindex |= RtoB(p->from.type-D_INDIR);
	if(p->from.index != D_NONE)
		info->regindex |= RtoB(p->from.index);
	if(p->to.type >= D_INDIR)
		info->regindex |= RtoB(p->to.type-D_INDIR);
	if(p->to.index != D_NONE)
		info->regindex |= RtoB(p->to.index);
}
