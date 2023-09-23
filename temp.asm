	.text
	.file	"ex1.cpp"
	.globl	_Z3addii
	.p2align	2
	.type	_Z3addii,@function
_Z3addii:
.L_Z3addii$local:
	add	w8, w1, w0
	lsl	w9, w0, #1
	cmp	w0, #0
	csel	w8, w8, w9, gt
	add	w0, w8, #3
	ret
.Lfunc_end0:
	.size	_Z3addii, .Lfunc_end0-_Z3addii

	.ident	"Ubuntu clang version 15.0.7"
	.section	".note.GNU-stack","",@progbits
