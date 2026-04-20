
demo_outside:     file format elf64-littleriscv


Disassembly of section .plt:

00000000000007b0 <.plt>:
 7b0:	00002397          	auipc	t2,0x2
 7b4:	41c30333          	sub	t1,t1,t3
 7b8:	8583be03          	ld	t3,-1960(t2) # 2008 <__TMC_END__>
 7bc:	fd430313          	addi	t1,t1,-44
 7c0:	85838293          	addi	t0,t2,-1960
 7c4:	00135313          	srli	t1,t1,0x1
 7c8:	0082b283          	ld	t0,8(t0)
 7cc:	000e0067          	jr	t3

00000000000007d0 <__libc_start_main@plt>:
 7d0:	00002e17          	auipc	t3,0x2
 7d4:	848e3e03          	ld	t3,-1976(t3) # 2018 <__libc_start_main@GLIBC_2.34>
 7d8:	000e0367          	jalr	t1,t3
 7dc:	00000013          	nop

00000000000007e0 <malloc@plt>:
 7e0:	00002e17          	auipc	t3,0x2
 7e4:	840e3e03          	ld	t3,-1984(t3) # 2020 <malloc@GLIBC_2.27>
 7e8:	000e0367          	jalr	t1,t3
 7ec:	00000013          	nop

00000000000007f0 <printf@plt>:
 7f0:	00002e17          	auipc	t3,0x2
 7f4:	838e3e03          	ld	t3,-1992(t3) # 2028 <printf@GLIBC_2.27>
 7f8:	000e0367          	jalr	t1,t3
 7fc:	00000013          	nop

0000000000000800 <free@plt>:
 800:	00002e17          	auipc	t3,0x2
 804:	830e3e03          	ld	t3,-2000(t3) # 2030 <free@GLIBC_2.27>
 808:	000e0367          	jalr	t1,t3
 80c:	00000013          	nop

Disassembly of section .text:

0000000000000810 <_start>:
 810:	022000ef          	jal	ra,832 <load_gp>
 814:	87aa                	mv	a5,a0
 816:	00002517          	auipc	a0,0x2
 81a:	83253503          	ld	a0,-1998(a0) # 2048 <_GLOBAL_OFFSET_TABLE_+0x10>
 81e:	6582                	ld	a1,0(sp)
 820:	0030                	addi	a2,sp,8
 822:	ff017113          	andi	sp,sp,-16
 826:	4681                	li	a3,0
 828:	4701                	li	a4,0
 82a:	880a                	mv	a6,sp
 82c:	fa5ff0ef          	jal	ra,7d0 <__libc_start_main@plt>
 830:	9002                	ebreak

0000000000000832 <load_gp>:
 832:	00002197          	auipc	gp,0x2
 836:	fce18193          	addi	gp,gp,-50 # 2800 <__global_pointer$>
 83a:	8082                	ret
	...

000000000000083e <deregister_tm_clones>:
 83e:	00001517          	auipc	a0,0x1
 842:	7ca50513          	addi	a0,a0,1994 # 2008 <__TMC_END__>
 846:	00001797          	auipc	a5,0x1
 84a:	7c278793          	addi	a5,a5,1986 # 2008 <__TMC_END__>
 84e:	00a78863          	beq	a5,a0,85e <deregister_tm_clones+0x20>
 852:	00001797          	auipc	a5,0x1
 856:	7ee7b783          	ld	a5,2030(a5) # 2040 <_ITM_deregisterTMCloneTable@Base>
 85a:	c391                	beqz	a5,85e <deregister_tm_clones+0x20>
 85c:	8782                	jr	a5
 85e:	8082                	ret

0000000000000860 <register_tm_clones>:
 860:	00001517          	auipc	a0,0x1
 864:	7a850513          	addi	a0,a0,1960 # 2008 <__TMC_END__>
 868:	00001597          	auipc	a1,0x1
 86c:	7a058593          	addi	a1,a1,1952 # 2008 <__TMC_END__>
 870:	8d89                	sub	a1,a1,a0
 872:	4035d793          	srai	a5,a1,0x3
 876:	91fd                	srli	a1,a1,0x3f
 878:	95be                	add	a1,a1,a5
 87a:	8585                	srai	a1,a1,0x1
 87c:	c599                	beqz	a1,88a <register_tm_clones+0x2a>
 87e:	00001797          	auipc	a5,0x1
 882:	7da7b783          	ld	a5,2010(a5) # 2058 <_ITM_registerTMCloneTable@Base>
 886:	c391                	beqz	a5,88a <register_tm_clones+0x2a>
 888:	8782                	jr	a5
 88a:	8082                	ret

000000000000088c <__do_global_dtors_aux>:
 88c:	1141                	addi	sp,sp,-16
 88e:	e022                	sd	s0,0(sp)
 890:	00001417          	auipc	s0,0x1
 894:	7d040413          	addi	s0,s0,2000 # 2060 <completed.0>
 898:	00044783          	lbu	a5,0(s0)
 89c:	e406                	sd	ra,8(sp)
 89e:	e385                	bnez	a5,8be <__do_global_dtors_aux+0x32>
 8a0:	00001797          	auipc	a5,0x1
 8a4:	7b07b783          	ld	a5,1968(a5) # 2050 <__cxa_finalize@GLIBC_2.27>
 8a8:	c791                	beqz	a5,8b4 <__do_global_dtors_aux+0x28>
 8aa:	00001517          	auipc	a0,0x1
 8ae:	75653503          	ld	a0,1878(a0) # 2000 <__dso_handle>
 8b2:	9782                	jalr	a5
 8b4:	f8bff0ef          	jal	ra,83e <deregister_tm_clones>
 8b8:	4785                	li	a5,1
 8ba:	00f40023          	sb	a5,0(s0)
 8be:	60a2                	ld	ra,8(sp)
 8c0:	6402                	ld	s0,0(sp)
 8c2:	0141                	addi	sp,sp,16
 8c4:	8082                	ret

00000000000008c6 <frame_dummy>:
 8c6:	bf69                	j	860 <register_tm_clones>

00000000000008c8 <main>:
 8c8:	1101                	addi	sp,sp,-32
 8ca:	ec06                	sd	ra,24(sp)
 8cc:	e822                	sd	s0,16(sp)
 8ce:	e426                	sd	s1,8(sp)
 8d0:	6505                	lui	a0,0x1
 8d2:	f0fff0ef          	jal	ra,7e0 <malloc@plt>
 8d6:	842a                	mv	s0,a0
 8d8:	6505                	lui	a0,0x1
 8da:	f07ff0ef          	jal	ra,7e0 <malloc@plt>
 8de:	84aa                	mv	s1,a0
 8e0:	4501                	li	a0,0
 8e2:	40000593          	li	a1,1024
 8e6:	0da07657          	vsetvli	a2,zero,e64,m4,ta,ma
 8ea:	5208a457          	vid.v	v8
 8ee:	3f800637          	lui	a2,0x3f800
 8f2:	0da5f6d7          	vsetvli	a3,a1,e64,m4,ta,ma
 8f6:	00251713          	slli	a4,a0,0x2
 8fa:	0280b657          	vadd.vi	v12,v8,1
 8fe:	00e407b3          	add	a5,s0,a4
 902:	9726                	add	a4,a4,s1
 904:	0d107057          	vsetvli	zero,zero,e32,m2,ta,ma
 908:	4ac91857          	vfncvt.f.xu.w	v16,v12
 90c:	5e064657          	vmv.v.x	v12,a2
 910:	9536                	add	a0,a0,a3
 912:	8d95                	sub	a1,a1,a3
 914:	02076627          	vse32.v	v12,(a4)
 918:	0207e827          	vse32.v	v16,(a5)
 91c:	0da07057          	vsetvli	zero,zero,e64,m4,ta,ma
 920:	0286c457          	vadd.vx	v8,v8,a3
 924:	f5f9                	bnez	a1,8f2 <main+0x2a>
 926:	4501                	li	a0,0
 928:	40000593          	li	a1,1024
 92c:	0d107657          	vsetvli	a2,zero,e32,m2,ta,ma
 930:	5e003457          	vmv.v.i	v8,0
 934:	0c75f657          	vsetvli	a2,a1,e8,mf2,ta,ma
 938:	00251693          	slli	a3,a0,0x2
 93c:	00d48733          	add	a4,s1,a3
 940:	96a2                	add	a3,a3,s0
 942:	02076507          	vle32.v	v10,(a4)
 946:	0206e607          	vle32.v	v12,(a3)
 94a:	8d91                	sub	a1,a1,a2
 94c:	09107057          	vsetvli	zero,zero,e32,m2,tu,ma
 950:	b2a61457          	vfmacc.vv	v8,v12,v10
 954:	9532                	add	a0,a0,a2
 956:	fdf9                	bnez	a1,934 <main+0x6c>
 958:	0d107557          	vsetvli	a0,zero,e32,m2,ta,ma
 95c:	42006557          	vmv.s.x	v10,zero
 960:	06851457          	vfredusum.vs	v8,v8,v10
 964:	428017d7          	vfmv.f.s	fa5,v8
 968:	420787d3          	fcvt.d.s	fa5,fa5
 96c:	e20785d3          	fmv.x.d	a1,fa5
 970:	00000517          	auipc	a0,0x0
 974:	02c50513          	addi	a0,a0,44 # 99c <_IO_stdin_used+0x4>
 978:	e79ff0ef          	jal	ra,7f0 <printf@plt>
 97c:	8522                	mv	a0,s0
 97e:	e83ff0ef          	jal	ra,800 <free@plt>
 982:	8526                	mv	a0,s1
 984:	e7dff0ef          	jal	ra,800 <free@plt>
 988:	4501                	li	a0,0
 98a:	60e2                	ld	ra,24(sp)
 98c:	6442                	ld	s0,16(sp)
 98e:	64a2                	ld	s1,8(sp)
 990:	6105                	addi	sp,sp,32
 992:	8082                	ret
	...
