	;NAM	TB01V137
*	WRITTEN 20-OCT-77 BY JOHN BYRNS
*	REVISED 30-DEC-77
*	REVISED 18-JAN-78
*	REVISED 10-APR-78
*	REVISED 08-MAY-79 TO ELIMINATE USE OF SP
*	REVISED 24-JAN-80 TO USE 6801 ON CHIP RAM
*	REVISED 26-JAN-80 FOR NEW 6801 INSTRUCTIONS
*	REVISED 24-JUL-81 FOR WHISTON BOARD
*	REVISED 24-SEP-81 INCLUDE USER FUNCTION
*	REVISED 08-APR-82 MAKE STANDALONE INCLUDE HEX CONSTANTS AND MEM FUNCTION
*	REVISED 21-NOV-84 FOR 6809
*	REVISED FEB 94 ADAPTED TO SIMULATOR AND BUGFIXES BY L.C. BENSCHOP.
*
EOL	EQU	$04
ETX	EQU	$03
SPACE	EQU	$20
CR	EQU	$0D
LF	EQU	$0A
BS	EQU	$08
CAN	EQU	$18
BELL	EQU	$07
FILL	EQU	$00
DEL	EQU	$7F
BSIZE	EQU	73
STKCUS	EQU	48
*
ACIA	EQU	$E000
RMCR	EQU	ACIA
TRCS	EQU	ACIA
RECEV	EQU	ACIA+1
TRANS	EQU	ACIA+1
CNTL1	EQU	$03
CNTL2	EQU	$15
RDRF	EQU	$01
ORFE	EQU	$20
TDRE	EQU	$02
* EDIT THE FOLLOWING EQUATES TO REFLECT THE
* DESIRED ROM AND RAM LAYOUT
LORAM	EQU	$0080	; ADDRESS OF DIRECT PAGE SCRATCH RAM
BUFFER	EQU	$4000	; ADDRESS OF MAIN RAM
RAMSIZ	EQU	$2000	; SIZE OF MAIN RAM
ROMADR	EQU	$400	; ADDRESS OF TINY BASIC ROM
*
RAMBEG	EQU	BUFFER+BSIZE
RAMEND	EQU	BUFFER+RAMSIZ
*
RAMPAT	EQU	$AA0F
ROMPAT	EQU	$F055
*
	ORG	LORAM
USRBAS	RMB	2
USRTOP	RMB	2
STKLIM	RMB	2
STKTOP	RMB	2
CURSOR	RMB	2
SAVESP	RMB	2
LINENB	RMB	2
SCRTCH	RMB	2
CHAR	RMB	2
ZONE	RMB	1
MODE	RMB	1
RESRVD	RMB	1
LOEND	EQU	*
*
	ORG     ROMADR
BASIC	JMP	SETUP
WARMS	LDS	STKTOP
	JSR	INTEEE
	BRA	WMS05
SETUP	LDS	#RAMEND-52
SET03	STS	STKTOP
	JSR	INTEEE
CLEAR	LDD	#RAMBEG
	STD	USRBAS
	STD	USRTOP
CLR02	STD	STKLIM
WMS05	JSR	CRLF
	LDX	#VSTR
	JSR	PUTSTR
CMDB	LDS	STKTOP
	CLR	MODE
	JSR	CRLF
	LDX	USRBAS
	STX	CURSOR
CMDE	LDX	#0000
	STX	LINENB
	TST	MODE
	BNE	CMD01
	LDA	#':
	JSR	PUTCHR
CMD01	JSR	GETLIN
	JSR	TSTNBR
	BCC	CMD02
	BVS	CMD05
	JSR	SKIPSP
	CMPA	#EOL
	BEQ	CMDE
	JSR	MSLINE
	BRA	CMDB
CMD02	PSHS	X
	LDX	USRTOP
	CMPX	STKLIM
	PULS	X
	BEQ	CMD03
	JMP	ERRORR
CMD03	ADDD	#0
	BEQ	CMD05
CMD04	PSHS	D
	SUBD	#9999
	PULS	D
	BHI	CMD05
	BSR	EDITOR
	BRA	CMDE
CMD05	JMP	ERRORS
VSTR	FCC	/TINY V1.37/
	FCB	EOL
******************************
******************************
EDITOR	PSHS	D
	JSR	SKIPSP
	STX	SCRTCH
	LDA	0,S
	LDX	CURSOR
	CMPX	USRTOP
	BEQ	ED00
	CMPD	0,X
	BCC	ED01
ED00	LDX	USRBAS
ED01	JSR	FNDLIN
	STX	CURSOR
	BCS	ED04
	STX	SAVESP
	LEAX	2,X
ED02	LDA	,X+
	CMPA	#EOL
	BNE	ED02
ED03	CMPX	USRTOP
	BEQ	ED35
	LDA	,X+
	STX	CHAR
	LDX	SAVESP
	STA	,X+
	STX	SAVESP
	LDX	CHAR
	BRA	ED03
ED35	LDX	SAVESP
	STX	USRTOP
	STX	STKLIM
ED04	LDX	SCRTCH
	LDB	#-1
ED05	INCB
	LDA	,X+
	CMPA	#EOL
	BNE	ED05
	TSTB
	BNE	ED55
	LEAS	2,S
	RTS
ED55	LEAX	-1,X
	ADDB	#4
ED06	LEAX	-1,X
	DECB
	LDA	0,X
	CMPA	#SPACE
	BEQ	ED06
	LDA	#EOL
	STA	1,X
	CLRA
	LDX	USRTOP
	STX	CHAR
	ADDD	USRTOP
	STD	USRTOP
	STD	STKLIM
	JSR	TSTSTK
	BCC	ED07
	STX	USRTOP
	STX	STKLIM
	JMP	ERRORF
ED07	LDX	USRTOP
ED08	STX	SAVESP
	LDX	CHAR
	CMPX	CURSOR
	BEQ	ED09
	LDA	,-X
	STX	CHAR
	LDX	SAVESP
	STA	,-X
	BRA	ED08
ED09	PULS	D
	LDX	CURSOR
	STD	,X++
	STX	CHAR
ED10	LDX	SCRTCH
	LDA	,X+
	STX	SCRTCH
	LDX	CHAR
	STA	,X+
	STX	CHAR
	CMPA	#EOL
	BNE	ED10
	RTS
******************************
******************************
PUTS01	JSR	PUTCHR
	LEAX	1,X
PUTSTR	LDA	0,X
	CMPA	#EOL
	BNE	PUTS01
	RTS
******************************
******************************
CRLF	LDX	#CRLFST
	BSR	PUTSTR
	CLR	ZONE
	RTS
CRLFST	FCB	CR,LF,DEL,FILL,FILL,FILL,EOL
******************************
******************************
ERRORF	BSR	ER01
	FCC	/SORRY/
	FCB	EOL
ERRORS	BSR	ER01
	FCC	/WHAT ?/
	FCB	EOL
ERRORR	BSR	ER01
	FCC	/HOW ?/
	FCB	EOL
BREAK	BSR	ER01
	FCC	/BREAK/
	FCB	EOL
END	BSR	ER01
	FCC	/STOP/
	FCB	EOL
ER01	BSR	CRLF
	LDA	#BELL
	JSR	PUTCHR
	LDD	LINENB
	JSR	PRNT4
	LDA	#SPACE
	JSR	PUTCHR
	PULS	X
	BSR	PUTSTR
	BSR	CRLF
	JMP	CMDB
******************************
******************************
GL00	BSR	CRLF
GETLIN	LDX	#BUFFER
GL03	JSR	GETCHR
	CMPA	#SPACE
	BCS	GL05
	CMPA	#$7F
	BEQ	GL03
	CMPX	#BUFFER+BSIZE-1
	BNE	GL04
	LDA	#BELL
	BRA	GL02
GL04	STA	,X+
GL02	JSR	PUTCHR
	BRA	GL03
GL05	CMPA	#BS
	BEQ	GL07
	CMPA	#CAN
	BEQ	GL00
	CMPA	#LF
	BEQ	GL09
	CMPA	#CR
	BNE	GL03
	TST	MODE
	BEQ	GL06
	JSR	PUTCHR
	BRA	GL08
GL06	PSHS	X
	JSR	CRLF
	PULS	X
GL08	LDA	#EOL
	STA	0,X
	LDX	#BUFFER
	RTS
GL07	CMPX	#BUFFER
	BEQ	GL03
	LEAX	-1,X
	LDA	#BS
	JSR	PUTCHR
	LDA	#SPACE
	JSR	PUTCHR
	LDA	#BS
	BRA	GL02
GL09	ORCC	#$01
	ROR	MODE
	BRA	GL02
******************************
******************************
REM00	LEAX	1,X
REM	BSR	SKIPSP
	CMPA	#EOL
	BNE	REM00
ENDSMT	JSR	TSTEOL
ENDS02	LDA	LINENB
	ORA	LINENB+1
	BEQ	REM09
REM05	CMPX	USRTOP
	BNE	NXTLIN
	JMP	ERRORR
NXTLIN	LDD	,X++
	STD	LINENB
MSLINE	JSR	TSTBRK
	BSR	IFAN
	BCS	IMPLET
	PSHS	D
REM09	RTS
IMPLET	JMP	LET
******************************
******************************
IFAN	BSR	SKIPSP
	STX	CURSOR
	LDX	#VERBT
FAN00	LDA	,X+
	CMPA	#EOL
	BNE	FAN04
	LDX	CURSOR
	ORCC	#$01
	RTS
FAN04	STX	CHAR
	LDX	CURSOR
	STX	SCRTCH
FAN05	LDX	SCRTCH
	CMPA	0,X
	BNE	FAN07
	LEAX	1,X
	STX	SCRTCH
	LDX	CHAR
	LDA	,X+
	STX	CHAR
	CMPA	#EOL
	BNE	FAN05
	LDD	0,X
	LDX	SCRTCH
	ANDCC	#$FE
	RTS
FAN07	LDX	CHAR
FAN08	LDA	,X+
	CMPA	#EOL
	BNE	FAN08
	LEAX	2,X
	BRA	FAN00
******************************
******************************
NXTNSP	LEAX	1,X
SKIPSP	LDA	0,X
	CMPA	#SPACE
	BEQ	NXTNSP
	RTS
******************************
******************************
TSTHEX	BSR	TSTDIG
	BCC	TST05
	CMPA	#'A
	BCS	TST03
	CMPA	#'F
	BHI	TST03
	SUBA	#'A-10
	ANDCC	#$FE
	RTS
******************************
******************************
TSTLTR	CMPA	#'A
	BCS	TST03
	CMPA	#'Z
	BLS	TST05
TST03	ORCC	#$01
	RTS
******************************
******************************
TSTDIG	CMPA	#'0
	BCS	TST03
	CMPA	#'9
	BHI	TST03
	SUBA	#'0
TST05	ANDCC	#$FE
	RTS
******************************
******************************
TSTVAR	BSR	SKIPSP
	BSR	TSTLTR
	BCS	TSTV03
	TFR	A,B
	LDA	1,X
	BSR	TSTLTR
	BCC	TST03
	LEAX	1,X
	SUBB	#'A
	ASLB
	CLRA
	ADDD	STKTOP
TSTV02	ANDCC	#$FE
TSTV03	RTS
******************************
******************************
USER	JSR	ARGONE
	PSHS	D
	JSR	SKIPSP
	CMPA	#',
	BEQ	USER03
	CMPA	#')
	ORCC	#$01
	BEQ	USER05
USER02	JMP	ERRORS
USER03	LEAX	1,X
	JSR	EXPR
	PSHS	A
	JSR	SKIPSP
	CMPA	#')
	PULS	A
	BNE	USER02
	ANDCC	#$FE
USER05	LEAX	1,X
	STX	CURSOR
	JSR	[,S++]
	LDX	CURSOR
	ANDCC	#$FE
	RTS
******************************
******************************
TSTSNB	JSR	SKIPSP
	CMPA	#'-
	BNE	TSTNBR
	LEAX	1,X
	BSR	TSTNBR
	BCS	TSN02
	NEGA
	NEGB
	SBCA	#0
	ANDCC	#$FC
TSN02	RTS
******************************
******************************
TSTNBR	JSR	SKIPSP
	JSR	TSTDIG
	BCC	TSTN02
	CMPA	#'$
	ORCC	#$01
	BNE	TSTN09
TSTN20	LEAX	1,X
	CLR	,-S
	CLR	,-S
TSTN23	LDA	0,X
	JSR	TSTHEX
	BCS	TSTN07
	LEAX	1,X
	PSHS	X
	PSHS	A
	LDD	3,S
	BITA	#$F0
	BNE	TSTN11
	ASLB
	ROLA
	ASLB
	ROLA
	ASLB
	ROLA
	ASLB
	ROLA
	ADDB	,S+
	STD	2,S
	PULS	X
	BRA	TSTN23
TSTN02	LEAX	1,X
	PSHS	A
	CLR	,-S
TSTN03	LDA	0,X
	JSR	TSTDIG
	BCS	TSTN07
	LEAX	1,X
	PSHS	X
	PSHS	A
	LDD	3,S
	ASLB
	ROLA
	BVS	TSTN11
	ASLB
	ROLA
	BVS	TSTN11
	ADDD	3,S
	BVS	TSTN11
	ASLB
	ROLA
	BVS	TSTN11
	ADDB	0,S
	ADCA	#0
	BVS	TSTN11
	STD	3,S
	LEAS	1,S
	PULS	X
	BRA	TSTN03
TSTN07	PULS	D
	ANDCC	#$FE
TSTN09	ANDCC	#$FD
	RTS
TSTN11	LDX	1,S
	LEAS	5,S
	ORCC	#$03
	RTS
******************************
******************************
TSTSTK	STS	SAVESP
	LDD	SAVESP
	SUBD	#STKCUS
	SUBD	STKLIM
	RTS
******************************
******************************
PEEK	JSR	PAREXP
	PSHS	D
	PSHS	X
	LDB	[2,S]
	PULS	X
	LEAS	2,S
	CLRA
	RTS
******************************
******************************
POKE	JSR	PAREXP
	PSHS	D
	JSR	SKIPSP
	CMPA	#'=
	BEQ	POKE05
	JMP	ERRORS
POKE05	LEAX	1,X
	JSR	EXPR
	JSR	TSTEOL
	PSHS	X
	STB	[2,S]
	PULS	X
	LEAS	2,S
	JMP	ENDS02
******************************
******************************
TSTFUN	JSR	SKIPSP
	STX	CURSOR
	LDX	#FUNT
	JSR	FAN00
	BCS	TSTF05
	PSHS	D
TSTF05	RTS
******************************
******************************
FUNT	FCC	/USR/
	FCB	EOL
	FDB	USER
	FCC	/PEEK/
	FCB	EOL
	FDB	PEEK
	FCC	/MEM/
	FCB	EOL
	FDB	TSTSTK
	FCB	EOL
******************************
******************************
FLINE	LDX	USRBAS
FNDLIN	CMPX	USRTOP
	BNE	FND03
	ORCC	#$03
	RTS
FND03	CMPD	0,X
	BNE	FND05
	ANDCC	#$FC
	RTS
FND05	BCC	FND07
	ORCC	#$01
	ANDCC	#$FD
	RTS
FND07	PSHS	A
	LDA	#EOL
	LEAX	1,X
FND09	LEAX	1,X
	CMPA	0,X
	BNE	FND09
	PULS	A
	LEAX	1,X
	BRA	FNDLIN
******************************
******************************
RELEXP	BSR	EXPR
	PSHS	D
	CLRB
	JSR	SKIPSP
	CMPA	#'=
	BEQ	REL06
	CMPA	#'<
	BNE	REL03
	LEAX	1,X
	INCB
	JSR	SKIPSP
	CMPA	#'>
	BNE	REL05
	LEAX	1,X
	ADDB	#4
	BRA	REL07
REL03	CMPA	#'>
	BNE	EXPR06
	LEAX	1,X
	ADDB	#4
	JSR	SKIPSP
REL05	CMPA	#'=
	BNE	REL07
REL06	LEAX	1,X
	ADDB	#2
REL07	PSHS	B
	BSR	EXPR
	PSHS	X
	SUBD	3,S
	TFR	CC,A
	LSRA
	TFR	A,B
	ASLA
	ASLA
	PSHS	B
	ADDA	,S+
	ANDA	#$06
	BNE	REL08
	INCA
REL08	CLRB
	ANDA	2,S
	BEQ	REL09
	COMB
REL09	CLRA
	PULS	X
	LEAS	3,S
	RTS
******************************
******************************
EXPR	CLR	,-S
	CLR	,-S
	JSR	SKIPSP
	CMPA	#'-
	BEQ	EXPR05
	CMPA	#'+
	BNE	EXPR03
EXPR02	LEAX	1,X
EXPR03	BSR	TERM
EXPR04	ADDD	0,S
	STD	0,S
	JSR	SKIPSP
	CMPA	#'+
	BEQ	EXPR02
	CMPA	#'-
	BNE	EXPR06
EXPR05	LEAX	1,X
	BSR	TERM
	NEGA
	NEGB
	SBCA	#0
	BRA	EXPR04
EXPR06	PULS	D
	RTS
******************************
******************************
TERM	JSR	FACT
	PSHS	D
TERM03	JSR	SKIPSP
	CMPA	#'*
	BEQ	TERM07
	CMPA	#'/
	BEQ	TERM05
	PULS	D
	RTS
TERM05	LEAX	1,X
	BSR	FACT
	PSHS	X
	LEAX	2,S
	PSHS	D
	EORA	0,X
	JSR	ABSX
	LEAX	0,S
	JSR	ABSX
	PSHS	A
	LDA	#17
	PSHS	A
	CLRA
	CLRB
DIV05	SUBD	2,S
	BCC	DIV07
	ADDD	2,S
	ANDCC	#$FE
	BRA	DIV09
DIV07	ORCC	#$01
DIV09	ROL	7,S
	ROL	6,S
	ROLB
	ROLA
	DEC	0,S
	BNE	DIV05
	LDA	1,S
	LEAS	4,S
	TSTA
	BPL	TERM06
	LEAX	2,S
	BSR	NEGX
TERM06	PULS	X
	BRA	TERM03
TERM07	LEAX	1,X
	BSR	FACT
MULT	PSHS	B
	LDB	2,S
	MUL
	LDA	1,S
	STB	1,S
	LDB	0,S
	MUL
	LDA	2,S
	STB	2,S
	PULS	B
	MUL
	ADDA	0,S
	ADDA	1,S
	STD	0,S
	BRA	TERM03
******************************
******************************
FACT	JSR	TSTVAR
	BCS	FACT03
	PSHS	X
	TFR	D,X
	LDD	0,X
	PULS	X
FACT02	RTS
FACT03	JSR	TSTNBR
	BCC	FACT02
	JSR	TSTFUN
	BCC	FACT02
PAREXP	BSR	ARGONE
	PSHS	A
	JSR	SKIPSP
	CMPA	#')
	PULS	A
	BNE	FACT05
	LEAX	1,X
	RTS
FACT05	JMP	ERRORS
******************************
******************************
ARGONE	JSR	TSTSTK
	BCC	FACT04
	JMP	ERRORF
FACT04	JSR	SKIPSP
	CMPA	#'(
	BNE	FACT05
	LEAX	1,X
	JMP	EXPR
******************************
******************************
ABSX	TST	0,X
	BPL	NEG05
NEGX	NEG	0,X
	NEG	1,X
	BCC	NEG05
	DEC	0,X
NEG05	RTS
******************************
******************************
TSTEOL	PSHS	A
	JSR	SKIPSP
	CMPA	#EOL
	BEQ	TEOL03
	JMP	ERRORS
TEOL03	LEAX	1,X
	PULS	A
	RTS
******************************
******************************
LET	JSR	TSTVAR
	BCC	LET03
	JMP	ERRORS
LET03	PSHS	D
	JSR	SKIPSP
	CMPA	#'=
	BEQ	LET05
	JMP	ERRORS
LET05	LEAX	1,X
	JSR	EXPR
	BSR	TSTEOL
	STX	CURSOR
	PULS	X
	STD	0,X
	LDX	CURSOR
	JMP	ENDS02
******************************
******************************
IF	JSR	RELEXP
	TSTB
	BEQ	IF03
	JMP	MSLINE
IF03	JMP	REM
******************************
******************************
GOTO	JSR	EXPR
	BSR	TSTEOL
	JSR	FLINE
	BCS	GOSB04
	JMP	NXTLIN
******************************
******************************
GOSUB	JSR	EXPR
	BSR	TSTEOL
	STX	CURSOR
	JSR	FLINE
	BCC	GOSB03
GOSB04	JMP	ERRORR
GOSB03	JSR	TSTSTK
	BCC	GOSB05
	JMP	ERRORF
GOSB05	LDD	CURSOR
	PSHS	D
	LDD	LINENB
	PSHS	D
	JSR	NXTLIN
	PULS	D
	STD	LINENB
	PULS	X
	JMP	ENDS02
******************************
******************************
RETURN	EQU	TSTEOL
******************************
******************************
PRINT	JSR	SKIPSP
PR01	CMPA	#',
	BEQ	PR05
	CMPA	#';
	BEQ	PR07
	CMPA	#EOL
	BEQ	PR04
	CMPA	#'"
	BNE	PR02
	LEAX	1,X
	BSR	PRNTQS
	BRA	PR03
PR02	JSR	EXPR
	PSHS	X
	BSR	PRNTN
	PULS	X
PR03	JSR	SKIPSP
	CMPA	#',
	BEQ	PR05
	CMPA	#';
	BEQ	PR07
	CMPA	#EOL
	BEQ	PR04
	JMP	ERRORS
PR04	PSHS	X
	JSR	CRLF
	PULS	X
	BRA	PR08
PR05	LDB	#$7
PR06	LDA	#SPACE
	JSR	PUTCHR
	BITB	ZONE
	BNE	PR06
PR07	LEAX	1,X
	JSR	SKIPSP
	CMPA	#EOL
	BNE	PR01
PR08	LEAX	1,X
	JMP	ENDS02
*
*
PRQ01	JSR	PUTCHR
PRNTQS	LDA	,X+
	CMPA	#EOL
	BNE	PRQ03
	JMP	ERRORS
PRQ03	CMPA	#'"
	BNE	PRQ01
	RTS
*
PRNTN	TSTA
	BPL	PRN03
	NEGA
	NEGB
	SBCA	#0
	PSHS	A
	LDA	#'-
	JSR	PUTCHR
	PULS	A
PRN03	LDX	#PRNPT-2
PRN05	LEAX	2,X
	CMPD	0,X
	BCC	PRN07
	CMPX	#PRNPTO
	BNE	PRN05
PRN07	CLR	CHAR
PRN09	CMPD	0,X
	BCS	PRN11
	SUBD	0,X
	INC	CHAR
	BRA	PRN09
PRN11	PSHS	A
	LDA	#'0
	ADDA	CHAR
	JSR	PUTCHR
	PULS	A
	CMPX	#PRNPTO
	BEQ	PRN13
	LEAX	2,X
	BRA	PRN07
PRN13	RTS
PRNPT	FDB	10000
	FDB	1000
	FDB	100
	FDB	10
PRNPTO	FDB	1
*
PRNT4	LDX	#PRNPT+2
	BRA	PRN07
******************************
******************************
INPUT	JSR	TSTVAR
	BCS	IN11
	PSHS	D
	STX	CURSOR
IN03	LDA	#'?
	JSR	PUTCHR
	JSR	GETLIN
IN05	JSR	SKIPSP
	CMPA	#EOL
	BEQ	IN03
	JSR	TSTSNB
	BCC	IN07
	LDX	#RMESS
	JSR	PUTSTR
	JSR	CRLF
	BRA	IN03
IN07	STX	SCRTCH
	PULS	X
	STD	0,X
	LDX	CURSOR
	JSR	SKIPSP
	CMPA	#',
	BEQ	IN09
	JMP	ENDSMT
IN09	LEAX	1,X
	JSR	TSTVAR
	BCC	IN13
IN11	JMP	ERRORS
IN13	PSHS	D
	PSHS	X
	LDX	SCRTCH
	JSR	SKIPSP
	CMPA	#',
	BNE	IN05
	LEAX	1,X
	BRA	IN05
RMESS	FCC	/RE-ENTER/
	FCB	EOL
******************************
******************************
RUN	LDX	STKTOP
	LDA	#52
RUN01	CLR	,X+
	DECA
	BNE	RUN01
	LDX	USRBAS
	JMP	REM05
******************************
******************************
LIST	JSR	TSTNBR
	BCC	LIST03
	CLRA
	CLRB
	STD	CURSOR
	LDA	#$7F
	BRA	LIST07
LIST03	STD	CURSOR
	JSR	SKIPSP
	CMPA	#',
	BEQ	LIST05
	LDA	CURSOR
	BRA	LIST07
LIST05	LEAX	1,X
	JSR	TSTNBR
	BCC	LIST07
	JMP	ERRORS
LIST07	JSR	TSTEOL
	PSHS	D
	LDD	CURSOR
	STX	CURSOR
	JSR	FLINE
LIST09	CMPX	USRTOP
	BEQ	LIST10
	PULS	D
	CMPD	0,X
	BCS	LIST11
	PSHS	D
	LDD	,X++
	PSHS	X
	JSR	PRNT4
	PULS	X
	LDA	#SPACE
	JSR	PUTCHR
	JSR	PUTSTR
	LEAX	1,X
	PSHS	X
	JSR	CRLF
	PULS	X
	JSR	TSTBRK
	BRA	LIST09
LIST10	LEAS	2,S
	LDA	#ETX
	JSR	PUTCHR
LIST11	LDX	CURSOR
	JMP	ENDS02
******************************
******************************
VERBT	FCC	/LET/
	FCB	EOL
	FDB	LET
	FCC	/IF/
	FCB	EOL
	FDB	IF
	FCC	/GOTO/
	FCB	EOL
	FDB	GOTO
	FCC	/GOSUB/
	FCB	EOL
	FDB	GOSUB
	FCC	/RETURN/
	FCB	EOL
	FDB	RETURN
	FCC	/POKE/
	FCB	EOL
	FDB	POKE
	FCC	/PRINT/
	FCB	EOL
	FDB	PRINT
	FCC	/INPUT/
	FCB	EOL
	FDB	INPUT
	FCC	/REM/
	FCB	EOL
	FDB	REM
	FCC	/STOP/
	FCB	EOL
	FDB	END
	FCC	/END/
	FCB	EOL
	FDB	END
	FCC	/RUN/
	FCB	EOL
	FDB	RUN
	FCC	/LIST/
	FCB	EOL
	FDB	LIST
	FCC	/NEW/
	FCB	EOL
	FDB	CLEAR
	FCC	/?/
	FCB	EOL
	FDB	PRINT
	FCB	EOL
******************************
******************************
TSTBRK	bsr	BRKEEE 	
	beq	GETC05
GETCHR	bsr 	INEEE
	CMPA	#ETX
	BNE	GETC05
	JMP	BREAK
GETC05	RTS
PUTCHR	INC	ZONE
	JMP	OUTEEE
******************************
******************************
INEEE	BSR	BRKEEE
	BEQ	INEEE
	LDA	RECEV
	ANDA	#$7F
	RTS
OUTEEE	PSHS	A
OUT01	LDA	TRCS
	BITA	#TDRE
	BEQ	OUT01
	PULS	A
	STA	TRANS
	RTS
BRKEEE	PSHS	A
BRK03	LDA	TRCS
	BITA	#ORFE
	BEQ	BRK05
	LDA	RECEV
	BRA	BRK03
BRK05	BITA	#RDRF
	PULS	A
	RTS
*
	LDA	#CNTL1
	STA	RMCR
	LDA	#CNTL2
	STA	TRCS
INTEEE  EQU     *
	RTS



******************************
******************************
	END ROMADR
