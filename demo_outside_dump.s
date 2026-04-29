
demo_outside：     文件格式 elf64-littleriscv


Disassembly of section .plt:

00000000000008e0 <.plt>:
 8e0:	00001397          	auipc	t2,0x1
 8e4:	41c30333          	sub	t1,t1,t3
 8e8:	6a03be03          	ld	t3,1696(t2) # 1f80 <.got>
 8ec:	fd430313          	addi	t1,t1,-44
 8f0:	6a038293          	addi	t0,t2,1696
 8f4:	00135313          	srli	t1,t1,0x1
 8f8:	0082b283          	ld	t0,8(t0)
 8fc:	000e0067          	jr	t3

0000000000000900 <fprintf@plt>:
 900:	00001e17          	auipc	t3,0x1
 904:	690e3e03          	ld	t3,1680(t3) # 1f90 <fprintf@GLIBC_2.27>
 908:	000e0367          	jalr	t1,t3
 90c:	00000013          	nop

0000000000000910 <__libc_start_main@plt>:
 910:	00001e17          	auipc	t3,0x1
 914:	688e3e03          	ld	t3,1672(t3) # 1f98 <__libc_start_main@GLIBC_2.34>
 918:	000e0367          	jalr	t1,t3
 91c:	00000013          	nop

0000000000000920 <malloc@plt>:
 920:	00001e17          	auipc	t3,0x1
 924:	680e3e03          	ld	t3,1664(t3) # 1fa0 <malloc@GLIBC_2.27>
 928:	000e0367          	jalr	t1,t3
 92c:	00000013          	nop

0000000000000930 <getpid@plt>:
 930:	00001e17          	auipc	t3,0x1
 934:	678e3e03          	ld	t3,1656(t3) # 1fa8 <getpid@GLIBC_2.27>
 938:	000e0367          	jalr	t1,t3
 93c:	00000013          	nop

0000000000000940 <perror@plt>:
 940:	00001e17          	auipc	t3,0x1
 944:	670e3e03          	ld	t3,1648(t3) # 1fb0 <perror@GLIBC_2.27>
 948:	000e0367          	jalr	t1,t3
 94c:	00000013          	nop

0000000000000950 <sleep@plt>:
 950:	00001e17          	auipc	t3,0x1
 954:	668e3e03          	ld	t3,1640(t3) # 1fb8 <sleep@GLIBC_2.27>
 958:	000e0367          	jalr	t1,t3
 95c:	00000013          	nop

0000000000000960 <printf@plt>:
 960:	00001e17          	auipc	t3,0x1
 964:	660e3e03          	ld	t3,1632(t3) # 1fc0 <printf@GLIBC_2.27>
 968:	000e0367          	jalr	t1,t3
 96c:	00000013          	nop

0000000000000970 <fclose@plt>:
 970:	00001e17          	auipc	t3,0x1
 974:	658e3e03          	ld	t3,1624(t3) # 1fc8 <fclose@GLIBC_2.27>
 978:	000e0367          	jalr	t1,t3
 97c:	00000013          	nop

0000000000000980 <fopen@plt>:
 980:	00001e17          	auipc	t3,0x1
 984:	650e3e03          	ld	t3,1616(t3) # 1fd0 <fopen@GLIBC_2.27>
 988:	000e0367          	jalr	t1,t3
 98c:	00000013          	nop

Disassembly of section .text:

0000000000000990 <_start>:
 990:	022000ef          	jal	9b2 <load_gp>
 994:	87aa                	mv	a5,a0
 996:	00001517          	auipc	a0,0x1
 99a:	65253503          	ld	a0,1618(a0) # 1fe8 <_GLOBAL_OFFSET_TABLE_+0x10>
 99e:	6582                	ld	a1,0(sp)
 9a0:	0030                	addi	a2,sp,8
 9a2:	ff017113          	andi	sp,sp,-16
 9a6:	4681                	li	a3,0
 9a8:	4701                	li	a4,0
 9aa:	880a                	mv	a6,sp
 9ac:	f65ff0ef          	jal	910 <__libc_start_main@plt>
 9b0:	9002                	ebreak

00000000000009b2 <load_gp>:
 9b2:	00002197          	auipc	gp,0x2
 9b6:	e4e18193          	addi	gp,gp,-434 # 2800 <__global_pointer$>
 9ba:	8082                	ret
 9bc:	0001                	nop

00000000000009be <deregister_tm_clones>:
 9be:	1141                	addi	sp,sp,-16
 9c0:	00001517          	auipc	a0,0x1
 9c4:	64850513          	addi	a0,a0,1608 # 2008 <completed.0>
 9c8:	00001797          	auipc	a5,0x1
 9cc:	64078793          	addi	a5,a5,1600 # 2008 <completed.0>
 9d0:	e022                	sd	s0,0(sp)
 9d2:	e406                	sd	ra,8(sp)
 9d4:	0800                	addi	s0,sp,16
 9d6:	00a78b63          	beq	a5,a0,9ec <deregister_tm_clones+0x2e>
 9da:	00001797          	auipc	a5,0x1
 9de:	6067b783          	ld	a5,1542(a5) # 1fe0 <_ITM_deregisterTMCloneTable>
 9e2:	c789                	beqz	a5,9ec <deregister_tm_clones+0x2e>
 9e4:	6402                	ld	s0,0(sp)
 9e6:	60a2                	ld	ra,8(sp)
 9e8:	0141                	addi	sp,sp,16
 9ea:	8782                	jr	a5
 9ec:	60a2                	ld	ra,8(sp)
 9ee:	6402                	ld	s0,0(sp)
 9f0:	0141                	addi	sp,sp,16
 9f2:	8082                	ret

00000000000009f4 <register_tm_clones>:
 9f4:	1141                	addi	sp,sp,-16
 9f6:	00001517          	auipc	a0,0x1
 9fa:	61250513          	addi	a0,a0,1554 # 2008 <completed.0>
 9fe:	00001597          	auipc	a1,0x1
 a02:	60a58593          	addi	a1,a1,1546 # 2008 <completed.0>
 a06:	e022                	sd	s0,0(sp)
 a08:	e406                	sd	ra,8(sp)
 a0a:	0800                	addi	s0,sp,16
 a0c:	8d89                	sub	a1,a1,a0
 a0e:	4035d793          	srai	a5,a1,0x3
 a12:	91fd                	srli	a1,a1,0x3f
 a14:	95be                	add	a1,a1,a5
 a16:	8585                	srai	a1,a1,0x1
 a18:	c991                	beqz	a1,a2c <register_tm_clones+0x38>
 a1a:	00001797          	auipc	a5,0x1
 a1e:	5de7b783          	ld	a5,1502(a5) # 1ff8 <_ITM_registerTMCloneTable>
 a22:	c789                	beqz	a5,a2c <register_tm_clones+0x38>
 a24:	6402                	ld	s0,0(sp)
 a26:	60a2                	ld	ra,8(sp)
 a28:	0141                	addi	sp,sp,16
 a2a:	8782                	jr	a5
 a2c:	60a2                	ld	ra,8(sp)
 a2e:	6402                	ld	s0,0(sp)
 a30:	0141                	addi	sp,sp,16
 a32:	8082                	ret

0000000000000a34 <__do_global_dtors_aux>:
 a34:	00001797          	auipc	a5,0x1
 a38:	5d47c783          	lbu	a5,1492(a5) # 2008 <completed.0>
 a3c:	eb95                	bnez	a5,a70 <__do_global_dtors_aux+0x3c>
 a3e:	1141                	addi	sp,sp,-16
 a40:	00001797          	auipc	a5,0x1
 a44:	5b07b783          	ld	a5,1456(a5) # 1ff0 <__cxa_finalize@GLIBC_2.27>
 a48:	e022                	sd	s0,0(sp)
 a4a:	e406                	sd	ra,8(sp)
 a4c:	0800                	addi	s0,sp,16
 a4e:	c791                	beqz	a5,a5a <__do_global_dtors_aux+0x26>
 a50:	00001517          	auipc	a0,0x1
 a54:	5b053503          	ld	a0,1456(a0) # 2000 <__dso_handle>
 a58:	9782                	jalr	a5
 a5a:	f65ff0ef          	jal	9be <deregister_tm_clones>
 a5e:	6402                	ld	s0,0(sp)
 a60:	4785                	li	a5,1
 a62:	60a2                	ld	ra,8(sp)
 a64:	00001717          	auipc	a4,0x1
 a68:	5af70223          	sb	a5,1444(a4) # 2008 <completed.0>
 a6c:	0141                	addi	sp,sp,16
 a6e:	8082                	ret
 a70:	8082                	ret

0000000000000a72 <frame_dummy>:
 a72:	1141                	addi	sp,sp,-16
 a74:	e022                	sd	s0,0(sp)
 a76:	e406                	sd	ra,8(sp)
 a78:	0800                	addi	s0,sp,16
 a7a:	6402                	ld	s0,0(sp)
 a7c:	60a2                	ld	ra,8(sp)
 a7e:	0141                	addi	sp,sp,16
 a80:	bf95                	j	9f4 <register_tm_clones>
	...

0000000000000a84 <call_migration>:
 a84:	1101                	addi	sp,sp,-32
 a86:	ec06                	sd	ra,24(sp)
 a88:	e822                	sd	s0,16(sp)
 a8a:	e426                	sd	s1,8(sp)
 a8c:	ea5ff0ef          	jal	930 <getpid@plt>
 a90:	84aa                	mv	s1,a0
 a92:	00000517          	auipc	a0,0x0
 a96:	00000597          	auipc	a1,0x0
 a9a:	15e50513          	addi	a0,a0,350 # bf0 <_IO_stdin_used+0x4>
 a9e:	16e58593          	addi	a1,a1,366 # c04 <_IO_stdin_used+0x18>
 aa2:	edfff0ef          	jal	980 <fopen@plt>
 aa6:	c50d                	beqz	a0,ad0 <call_migration+0x4c>
 aa8:	842a                	mv	s0,a0
 aaa:	4090063b          	negw	a2,s1
 aae:	00000597          	auipc	a1,0x0
 ab2:	18158593          	addi	a1,a1,385 # c2f <_IO_stdin_used+0x43>
 ab6:	e4bff0ef          	jal	900 <fprintf@plt>
 aba:	02054463          	bltz	a0,ae2 <call_migration+0x5e>
 abe:	8522                	mv	a0,s0
 ac0:	eb1ff0ef          	jal	970 <fclose@plt>
 ac4:	e91d                	bnez	a0,afa <call_migration+0x76>
 ac6:	60e2                	ld	ra,24(sp)
 ac8:	6442                	ld	s0,16(sp)
 aca:	64a2                	ld	s1,8(sp)
 acc:	6105                	addi	sp,sp,32
 ace:	8082                	ret
 ad0:	00000517          	auipc	a0,0x0
 ad4:	13650513          	addi	a0,a0,310 # c06 <_IO_stdin_used+0x1a>
 ad8:	60e2                	ld	ra,24(sp)
 ada:	6442                	ld	s0,16(sp)
 adc:	64a2                	ld	s1,8(sp)
 ade:	6105                	addi	sp,sp,32
 ae0:	b585                	j	940 <perror@plt>
 ae2:	00000517          	auipc	a0,0x0
 ae6:	15050513          	addi	a0,a0,336 # c32 <_IO_stdin_used+0x46>
 aea:	e57ff0ef          	jal	940 <perror@plt>
 aee:	8522                	mv	a0,s0
 af0:	60e2                	ld	ra,24(sp)
 af2:	6442                	ld	s0,16(sp)
 af4:	64a2                	ld	s1,8(sp)
 af6:	6105                	addi	sp,sp,32
 af8:	bda5                	j	970 <fclose@plt>
 afa:	00000517          	auipc	a0,0x0
 afe:	16a50513          	addi	a0,a0,362 # c64 <_IO_stdin_used+0x78>
 b02:	60e2                	ld	ra,24(sp)
 b04:	6442                	ld	s0,16(sp)
 b06:	64a2                	ld	s1,8(sp)
 b08:	6105                	addi	sp,sp,32
 b0a:	bd1d                	j	940 <perror@plt>

0000000000000b0c <main>:
 b0c:	7179                	addi	sp,sp,-48
 b0e:	f406                	sd	ra,40(sp)
 b10:	f022                	sd	s0,32(sp)
 b12:	ec26                	sd	s1,24(sp)
 b14:	e84a                	sd	s2,16(sp)
 b16:	a422                	fsd	fs0,8(sp)
 b18:	6505                	lui	a0,0x1
 b1a:	e07ff0ef          	jal	920 <malloc@plt>
 b1e:	842a                	mv	s0,a0
 b20:	6505                	lui	a0,0x1
 b22:	dffff0ef          	jal	920 <malloc@plt>
 b26:	84aa                	mv	s1,a0
 b28:	4501                	li	a0,0
 b2a:	40000593          	li	a1,1024
 b2e:	0da07657          	vsetvli	a2,zero,e64,m4,ta,ma
 b32:	5208a457          	vid.v	v8
 b36:	3f800637          	lui	a2,0x3f800
 b3a:	0da5f6d7          	vsetvli	a3,a1,e64,m4,ta,ma
 b3e:	00251713          	slli	a4,a0,0x2
 b42:	0280b657          	vadd.vi	v12,v8,1
 b46:	00e407b3          	add	a5,s0,a4
 b4a:	9726                	add	a4,a4,s1
 b4c:	0d107057          	vsetvli	zero,zero,e32,m2,ta,ma
 b50:	4ac91857          	vfncvt.f.xu.w	v16,v12
 b54:	5e064657          	vmv.v.x	v12,a2
 b58:	9536                	add	a0,a0,a3
 b5a:	8d95                	sub	a1,a1,a3
 b5c:	02076627          	vse32.v	v12,(a4)
 b60:	0207e827          	vse32.v	v16,(a5)
 b64:	0da07057          	vsetvli	zero,zero,e64,m4,ta,ma
 b68:	0286c457          	vadd.vx	v8,v8,a3
 b6c:	f5f9                	bnez	a1,b3a <main+0x2e>
 b6e:	f17ff0ef          	jal	a84 <call_migration>
 b72:	4501                	li	a0,0
 b74:	40000593          	li	a1,1024
 b78:	f0000453          	fmv.w.x	fs0,zero
 b7c:	00000917          	auipc	s2,0x0
 b80:	11890913          	addi	s2,s2,280 # c94 <_IO_stdin_used+0xa8>
 b84:	208404d3          	fmv.s	fs1,fs0
 b88:	4501                	li	a0,0
 b8a:	40000593          	li	a1,1024
 b8e:	0d107657          	vsetvli	a2,zero,e32,m2,ta,ma
 b92:	5e003457          	vmv.v.i	v8,0
 b96:	0c75f657          	vsetvli	a2,a1,e8,mf2,ta,ma
 b9a:	00251693          	slli	a3,a0,0x2
 b9e:	00d48733          	add	a4,s1,a3
 ba2:	96a2                	add	a3,a3,s0
 ba4:	02076507          	vle32.v	v10,(a4)
 ba8:	0206e607          	vle32.v	v12,(a3)
 bac:	8d91                	sub	a1,a1,a2
 bae:	09107057          	vsetvli	zero,zero,e32,m2,tu,ma
 bb2:	b2a61457          	vfmacc.vv	v8,v12,v10
 bb6:	9532                	add	a0,a0,a2
 bb8:	fdf9                	bnez	a1,b96 <main+0x8a>
 bba:	0d107557          	vsetvli	a0,zero,e32,m2,ta,ma
 bbe:	42006557          	vmv.s.x	v10,zero
 bc2:	06851457          	vfredusum.vs	v8,v8,v10
 bc6:	428017d7          	vfmv.f.s	fa5,v8
 bca:	420787d3          	fcvt.d.s	fa5,fa5
 bce:	e20785d3          	fmv.x.d	a1,fa5
 bd2:	854a                	mv	a0,s2
 bd4:	d8dff0ef          	jal	960 <printf@plt>
 bd8:	4505                	li	a0,1
 bda:	d77ff0ef          	jal	950 <sleep@plt>
 bde:	4501                	li	a0,0
 be0:	208404d3          	fmv.s	fs1,fs0
 be4:	40000593          	li	a1,1024
 be8:	b745                	j	b88 <main+0x7c>
 bea:	0001                	nop
