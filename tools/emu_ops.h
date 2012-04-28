#define	op_MFUNC_MASK	0060000

#define	op_MFUNC_JUMP	0000000
#define	op_JUMP_MASK	0014000

#define	op_JMP		0000000
#define	op_JSR		0004000
#define	op_ISZ		0010000
#define	op_DSZ		0014000

#define	op_LDA		0020000
#define	op_STA		0040000
#define	op_AUGMENTED	0060000

#define	op_AUGM_MASK	0177400	/* mask covering all augmented functions */
#define	op_AUGM_NODISP	0061000	/* augmented functions w/o displacement */
#define	op_AUGM_SUBFUNC	0000037	/* mask for augmented subfunctions in DISP */

#define	op_CYCLE	0060000	/* rotate left AC0 disp.3-0 times (or AC1 times, if 0) */
#define	op_DIR		0061000
#define	op_EIR		0061001
#define	op_BRI		0061002
#define	op_RCLK		0061003
#define	op_SIO		0061004
#define	op_BLT		0061005
#define	op_BLKS		0061006
#define	op_SIT		0061007
#define	op_JMPRAM	0061010
#define	op_RDRAM	0061011
#define	op_WRTRAM	0061012
#define	op_DIRS		0061013
#define	op_VERS		0061014
#define	op_DREAD	0061015
#define	op_DWRITE	0061016
#define	op_DEXCH	0061017
#define	op_MUL		0061020
#define	op_DIV		0061021
#define	op_DIAGNOSE1	0061022
#define	op_DIAGNOSE2	0061023
#define	op_BITBLT	0061024
#define	op_XMLDA	0061026
#define	op_XMSTA	0061027
#define	op_JSRII	0064400
#define	op_JSRIS	0065000
#define	op_CONVERT	0067000

#define	op_ARITH_MASK	0103400
#define	op_COM		0100000
#define	op_NEG		0100400
#define	op_MOV		0101000
#define	op_INC		0101400
#define	op_ADC		0102000
#define	op_SUB		0102400
#define	op_ADD		0103000
#define	op_AND		0103400

#define	ea_DIRECT	0000000
#define	ea_INDIRECT	0002000

#define	ea_MASK		0001400
#define	ea_PAGE0	0000000
#define	ea_PCREL	0000400
#define	ea_AC2REL	0001000
#define	ea_AC3REL	0001400

#define	sh_MASK		0000300
#define	sh_L		0000100
#define	sh_R		0000200
#define	sh_S		0000300

#define	cy_MASK		0000060
#define	cy_Z		0000020
#define	cy_O		0000040
#define	cy_C		0000060

#define	nl_MASK		0000010
#define	nl_		0000010

#define	sk_MASK		0000007
#define	sk_NVR		0000000	/* never skip */
#define	sk_SKP		0000001	/* always skip */
#define	sk_SZC		0000002	/* skip if carry result is zero */
#define	sk_SNC		0000003	/* skip if carry result is non-zero */
#define	sk_SZR		0000004	/* skip if 16-bit result is zero */
#define	sk_SNR		0000005	/* skip if 16-bit result is non-zero */
#define	sk_SEZ		0000006	/* skip if either result is zero */
#define	sk_SBN		0000007	/* skip if both results are non-zero */
