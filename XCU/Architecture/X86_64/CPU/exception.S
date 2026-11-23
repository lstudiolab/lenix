.section .text
.extern nux_arch_exception_dispatch
.global nux_isr_0
.type nux_isr_0,@function
nux_isr_0:
    pushq $0
    pushq $0
    jmp nux_isr_common
.size nux_isr_0, .-nux_isr_0
.global nux_isr_1
.type nux_isr_1,@function
nux_isr_1:
    pushq $0
    pushq $1
    jmp nux_isr_common
.size nux_isr_1, .-nux_isr_1
.global nux_isr_2
.type nux_isr_2,@function
nux_isr_2:
    pushq $0
    pushq $2
    jmp nux_isr_common
.size nux_isr_2, .-nux_isr_2
.global nux_isr_3
.type nux_isr_3,@function
nux_isr_3:
    pushq $0
    pushq $3
    jmp nux_isr_common
.size nux_isr_3, .-nux_isr_3
.global nux_isr_4
.type nux_isr_4,@function
nux_isr_4:
    pushq $0
    pushq $4
    jmp nux_isr_common
.size nux_isr_4, .-nux_isr_4
.global nux_isr_5
.type nux_isr_5,@function
nux_isr_5:
    pushq $0
    pushq $5
    jmp nux_isr_common
.size nux_isr_5, .-nux_isr_5
.global nux_isr_6
.type nux_isr_6,@function
nux_isr_6:
    pushq $0
    pushq $6
    jmp nux_isr_common
.size nux_isr_6, .-nux_isr_6
.global nux_isr_7
.type nux_isr_7,@function
nux_isr_7:
    pushq $0
    pushq $7
    jmp nux_isr_common
.size nux_isr_7, .-nux_isr_7
.global nux_isr_8
.type nux_isr_8,@function
nux_isr_8:
    pushq $8
    jmp nux_isr_common
.size nux_isr_8, .-nux_isr_8
.global nux_isr_9
.type nux_isr_9,@function
nux_isr_9:
    pushq $0
    pushq $9
    jmp nux_isr_common
.size nux_isr_9, .-nux_isr_9
.global nux_isr_10
.type nux_isr_10,@function
nux_isr_10:
    pushq $10
    jmp nux_isr_common
.size nux_isr_10, .-nux_isr_10
.global nux_isr_11
.type nux_isr_11,@function
nux_isr_11:
    pushq $11
    jmp nux_isr_common
.size nux_isr_11, .-nux_isr_11
.global nux_isr_12
.type nux_isr_12,@function
nux_isr_12:
    pushq $12
    jmp nux_isr_common
.size nux_isr_12, .-nux_isr_12
.global nux_isr_13
.type nux_isr_13,@function
nux_isr_13:
    pushq $13
    jmp nux_isr_common
.size nux_isr_13, .-nux_isr_13
.global nux_isr_14
.type nux_isr_14,@function
nux_isr_14:
    pushq $14
    jmp nux_isr_common
.size nux_isr_14, .-nux_isr_14
.global nux_isr_15
.type nux_isr_15,@function
nux_isr_15:
    pushq $0
    pushq $15
    jmp nux_isr_common
.size nux_isr_15, .-nux_isr_15
.global nux_isr_16
.type nux_isr_16,@function
nux_isr_16:
    pushq $0
    pushq $16
    jmp nux_isr_common
.size nux_isr_16, .-nux_isr_16
.global nux_isr_17
.type nux_isr_17,@function
nux_isr_17:
    pushq $17
    jmp nux_isr_common
.size nux_isr_17, .-nux_isr_17
.global nux_isr_18
.type nux_isr_18,@function
nux_isr_18:
    pushq $0
    pushq $18
    jmp nux_isr_common
.size nux_isr_18, .-nux_isr_18
.global nux_isr_19
.type nux_isr_19,@function
nux_isr_19:
    pushq $0
    pushq $19
    jmp nux_isr_common
.size nux_isr_19, .-nux_isr_19
.global nux_isr_20
.type nux_isr_20,@function
nux_isr_20:
    pushq $0
    pushq $20
    jmp nux_isr_common
.size nux_isr_20, .-nux_isr_20
.global nux_isr_21
.type nux_isr_21,@function
nux_isr_21:
    pushq $21
    jmp nux_isr_common
.size nux_isr_21, .-nux_isr_21
.global nux_isr_22
.type nux_isr_22,@function
nux_isr_22:
    pushq $0
    pushq $22
    jmp nux_isr_common
.size nux_isr_22, .-nux_isr_22
.global nux_isr_23
.type nux_isr_23,@function
nux_isr_23:
    pushq $0
    pushq $23
    jmp nux_isr_common
.size nux_isr_23, .-nux_isr_23
.global nux_isr_24
.type nux_isr_24,@function
nux_isr_24:
    pushq $0
    pushq $24
    jmp nux_isr_common
.size nux_isr_24, .-nux_isr_24
.global nux_isr_25
.type nux_isr_25,@function
nux_isr_25:
    pushq $0
    pushq $25
    jmp nux_isr_common
.size nux_isr_25, .-nux_isr_25
.global nux_isr_26
.type nux_isr_26,@function
nux_isr_26:
    pushq $0
    pushq $26
    jmp nux_isr_common
.size nux_isr_26, .-nux_isr_26
.global nux_isr_27
.type nux_isr_27,@function
nux_isr_27:
    pushq $0
    pushq $27
    jmp nux_isr_common
.size nux_isr_27, .-nux_isr_27
.global nux_isr_28
.type nux_isr_28,@function
nux_isr_28:
    pushq $0
    pushq $28
    jmp nux_isr_common
.size nux_isr_28, .-nux_isr_28
.global nux_isr_29
.type nux_isr_29,@function
nux_isr_29:
    pushq $29
    jmp nux_isr_common
.size nux_isr_29, .-nux_isr_29
.global nux_isr_30
.type nux_isr_30,@function
nux_isr_30:
    pushq $30
    jmp nux_isr_common
.size nux_isr_30, .-nux_isr_30
.global nux_isr_31
.type nux_isr_31,@function
nux_isr_31:
    pushq $0
    pushq $31
    jmp nux_isr_common
.size nux_isr_31, .-nux_isr_31
.global nux_isr_32
.type nux_isr_32,@function
nux_isr_32:
    pushq $0
    pushq $32
    jmp nux_isr_common
.size nux_isr_32, .-nux_isr_32
.global nux_isr_33
.type nux_isr_33,@function
nux_isr_33:
    pushq $0
    pushq $33
    jmp nux_isr_common
.size nux_isr_33, .-nux_isr_33
.global nux_isr_34
.type nux_isr_34,@function
nux_isr_34:
    pushq $0
    pushq $34
    jmp nux_isr_common
.size nux_isr_34, .-nux_isr_34
.global nux_isr_35
.type nux_isr_35,@function
nux_isr_35:
    pushq $0
    pushq $35
    jmp nux_isr_common
.size nux_isr_35, .-nux_isr_35
.global nux_isr_36
.type nux_isr_36,@function
nux_isr_36:
    pushq $0
    pushq $36
    jmp nux_isr_common
.size nux_isr_36, .-nux_isr_36
.global nux_isr_37
.type nux_isr_37,@function
nux_isr_37:
    pushq $0
    pushq $37
    jmp nux_isr_common
.size nux_isr_37, .-nux_isr_37
.global nux_isr_38
.type nux_isr_38,@function
nux_isr_38:
    pushq $0
    pushq $38
    jmp nux_isr_common
.size nux_isr_38, .-nux_isr_38
.global nux_isr_39
.type nux_isr_39,@function
nux_isr_39:
    pushq $0
    pushq $39
    jmp nux_isr_common
.size nux_isr_39, .-nux_isr_39
.global nux_isr_40
.type nux_isr_40,@function
nux_isr_40:
    pushq $0
    pushq $40
    jmp nux_isr_common
.size nux_isr_40, .-nux_isr_40
.global nux_isr_41
.type nux_isr_41,@function
nux_isr_41:
    pushq $0
    pushq $41
    jmp nux_isr_common
.size nux_isr_41, .-nux_isr_41
.global nux_isr_42
.type nux_isr_42,@function
nux_isr_42:
    pushq $0
    pushq $42
    jmp nux_isr_common
.size nux_isr_42, .-nux_isr_42
.global nux_isr_43
.type nux_isr_43,@function
nux_isr_43:
    pushq $0
    pushq $43
    jmp nux_isr_common
.size nux_isr_43, .-nux_isr_43
.global nux_isr_44
.type nux_isr_44,@function
nux_isr_44:
    pushq $0
    pushq $44
    jmp nux_isr_common
.size nux_isr_44, .-nux_isr_44
.global nux_isr_45
.type nux_isr_45,@function
nux_isr_45:
    pushq $0
    pushq $45
    jmp nux_isr_common
.size nux_isr_45, .-nux_isr_45
.global nux_isr_46
.type nux_isr_46,@function
nux_isr_46:
    pushq $0
    pushq $46
    jmp nux_isr_common
.size nux_isr_46, .-nux_isr_46
.global nux_isr_47
.type nux_isr_47,@function
nux_isr_47:
    pushq $0
    pushq $47
    jmp nux_isr_common
.size nux_isr_47, .-nux_isr_47
.global nux_isr_48
.type nux_isr_48,@function
nux_isr_48:
    pushq $0
    pushq $48
    jmp nux_isr_common
.size nux_isr_48, .-nux_isr_48
.global nux_isr_49
.type nux_isr_49,@function
nux_isr_49:
    pushq $0
    pushq $49
    jmp nux_isr_common
.size nux_isr_49, .-nux_isr_49
.global nux_isr_50
.type nux_isr_50,@function
nux_isr_50:
    pushq $0
    pushq $50
    jmp nux_isr_common
.size nux_isr_50, .-nux_isr_50
.global nux_isr_51
.type nux_isr_51,@function
nux_isr_51:
    pushq $0
    pushq $51
    jmp nux_isr_common
.size nux_isr_51, .-nux_isr_51
.global nux_isr_52
.type nux_isr_52,@function
nux_isr_52:
    pushq $0
    pushq $52
    jmp nux_isr_common
.size nux_isr_52, .-nux_isr_52
.global nux_isr_53
.type nux_isr_53,@function
nux_isr_53:
    pushq $0
    pushq $53
    jmp nux_isr_common
.size nux_isr_53, .-nux_isr_53
.global nux_isr_54
.type nux_isr_54,@function
nux_isr_54:
    pushq $0
    pushq $54
    jmp nux_isr_common
.size nux_isr_54, .-nux_isr_54
.global nux_isr_55
.type nux_isr_55,@function
nux_isr_55:
    pushq $0
    pushq $55
    jmp nux_isr_common
.size nux_isr_55, .-nux_isr_55
.global nux_isr_56
.type nux_isr_56,@function
nux_isr_56:
    pushq $0
    pushq $56
    jmp nux_isr_common
.size nux_isr_56, .-nux_isr_56
.global nux_isr_57
.type nux_isr_57,@function
nux_isr_57:
    pushq $0
    pushq $57
    jmp nux_isr_common
.size nux_isr_57, .-nux_isr_57
.global nux_isr_58
.type nux_isr_58,@function
nux_isr_58:
    pushq $0
    pushq $58
    jmp nux_isr_common
.size nux_isr_58, .-nux_isr_58
.global nux_isr_59
.type nux_isr_59,@function
nux_isr_59:
    pushq $0
    pushq $59
    jmp nux_isr_common
.size nux_isr_59, .-nux_isr_59
.global nux_isr_60
.type nux_isr_60,@function
nux_isr_60:
    pushq $0
    pushq $60
    jmp nux_isr_common
.size nux_isr_60, .-nux_isr_60
.global nux_isr_61
.type nux_isr_61,@function
nux_isr_61:
    pushq $0
    pushq $61
    jmp nux_isr_common
.size nux_isr_61, .-nux_isr_61
.global nux_isr_62
.type nux_isr_62,@function
nux_isr_62:
    pushq $0
    pushq $62
    jmp nux_isr_common
.size nux_isr_62, .-nux_isr_62
.global nux_isr_63
.type nux_isr_63,@function
nux_isr_63:
    pushq $0
    pushq $63
    jmp nux_isr_common
.size nux_isr_63, .-nux_isr_63
.global nux_isr_64
.type nux_isr_64,@function
nux_isr_64:
    pushq $0
    pushq $64
    jmp nux_isr_common
.size nux_isr_64, .-nux_isr_64
.global nux_isr_65
.type nux_isr_65,@function
nux_isr_65:
    pushq $0
    pushq $65
    jmp nux_isr_common
.size nux_isr_65, .-nux_isr_65
.global nux_isr_66
.type nux_isr_66,@function
nux_isr_66:
    pushq $0
    pushq $66
    jmp nux_isr_common
.size nux_isr_66, .-nux_isr_66
.global nux_isr_67
.type nux_isr_67,@function
nux_isr_67:
    pushq $0
    pushq $67
    jmp nux_isr_common
.size nux_isr_67, .-nux_isr_67
.global nux_isr_68
.type nux_isr_68,@function
nux_isr_68:
    pushq $0
    pushq $68
    jmp nux_isr_common
.size nux_isr_68, .-nux_isr_68
.global nux_isr_69
.type nux_isr_69,@function
nux_isr_69:
    pushq $0
    pushq $69
    jmp nux_isr_common
.size nux_isr_69, .-nux_isr_69
.global nux_isr_70
.type nux_isr_70,@function
nux_isr_70:
    pushq $0
    pushq $70
    jmp nux_isr_common
.size nux_isr_70, .-nux_isr_70
.global nux_isr_71
.type nux_isr_71,@function
nux_isr_71:
    pushq $0
    pushq $71
    jmp nux_isr_common
.size nux_isr_71, .-nux_isr_71
.global nux_isr_72
.type nux_isr_72,@function
nux_isr_72:
    pushq $0
    pushq $72
    jmp nux_isr_common
.size nux_isr_72, .-nux_isr_72
.global nux_isr_73
.type nux_isr_73,@function
nux_isr_73:
    pushq $0
    pushq $73
    jmp nux_isr_common
.size nux_isr_73, .-nux_isr_73
.global nux_isr_74
.type nux_isr_74,@function
nux_isr_74:
    pushq $0
    pushq $74
    jmp nux_isr_common
.size nux_isr_74, .-nux_isr_74
.global nux_isr_75
.type nux_isr_75,@function
nux_isr_75:
    pushq $0
    pushq $75
    jmp nux_isr_common
.size nux_isr_75, .-nux_isr_75
.global nux_isr_76
.type nux_isr_76,@function
nux_isr_76:
    pushq $0
    pushq $76
    jmp nux_isr_common
.size nux_isr_76, .-nux_isr_76
.global nux_isr_77
.type nux_isr_77,@function
nux_isr_77:
    pushq $0
    pushq $77
    jmp nux_isr_common
.size nux_isr_77, .-nux_isr_77
.global nux_isr_78
.type nux_isr_78,@function
nux_isr_78:
    pushq $0
    pushq $78
    jmp nux_isr_common
.size nux_isr_78, .-nux_isr_78
.global nux_isr_79
.type nux_isr_79,@function
nux_isr_79:
    pushq $0
    pushq $79
    jmp nux_isr_common
.size nux_isr_79, .-nux_isr_79
.global nux_isr_80
.type nux_isr_80,@function
nux_isr_80:
    pushq $0
    pushq $80
    jmp nux_isr_common
.size nux_isr_80, .-nux_isr_80
.global nux_isr_81
.type nux_isr_81,@function
nux_isr_81:
    pushq $0
    pushq $81
    jmp nux_isr_common
.size nux_isr_81, .-nux_isr_81
.global nux_isr_82
.type nux_isr_82,@function
nux_isr_82:
    pushq $0
    pushq $82
    jmp nux_isr_common
.size nux_isr_82, .-nux_isr_82
.global nux_isr_83
.type nux_isr_83,@function
nux_isr_83:
    pushq $0
    pushq $83
    jmp nux_isr_common
.size nux_isr_83, .-nux_isr_83
.global nux_isr_84
.type nux_isr_84,@function
nux_isr_84:
    pushq $0
    pushq $84
    jmp nux_isr_common
.size nux_isr_84, .-nux_isr_84
.global nux_isr_85
.type nux_isr_85,@function
nux_isr_85:
    pushq $0
    pushq $85
    jmp nux_isr_common
.size nux_isr_85, .-nux_isr_85
.global nux_isr_86
.type nux_isr_86,@function
nux_isr_86:
    pushq $0
    pushq $86
    jmp nux_isr_common
.size nux_isr_86, .-nux_isr_86
.global nux_isr_87
.type nux_isr_87,@function
nux_isr_87:
    pushq $0
    pushq $87
    jmp nux_isr_common
.size nux_isr_87, .-nux_isr_87
.global nux_isr_88
.type nux_isr_88,@function
nux_isr_88:
    pushq $0
    pushq $88
    jmp nux_isr_common
.size nux_isr_88, .-nux_isr_88
.global nux_isr_89
.type nux_isr_89,@function
nux_isr_89:
    pushq $0
    pushq $89
    jmp nux_isr_common
.size nux_isr_89, .-nux_isr_89
.global nux_isr_90
.type nux_isr_90,@function
nux_isr_90:
    pushq $0
    pushq $90
    jmp nux_isr_common
.size nux_isr_90, .-nux_isr_90
.global nux_isr_91
.type nux_isr_91,@function
nux_isr_91:
    pushq $0
    pushq $91
    jmp nux_isr_common
.size nux_isr_91, .-nux_isr_91
.global nux_isr_92
.type nux_isr_92,@function
nux_isr_92:
    pushq $0
    pushq $92
    jmp nux_isr_common
.size nux_isr_92, .-nux_isr_92
.global nux_isr_93
.type nux_isr_93,@function
nux_isr_93:
    pushq $0
    pushq $93
    jmp nux_isr_common
.size nux_isr_93, .-nux_isr_93
.global nux_isr_94
.type nux_isr_94,@function
nux_isr_94:
    pushq $0
    pushq $94
    jmp nux_isr_common
.size nux_isr_94, .-nux_isr_94
.global nux_isr_95
.type nux_isr_95,@function
nux_isr_95:
    pushq $0
    pushq $95
    jmp nux_isr_common
.size nux_isr_95, .-nux_isr_95
.global nux_isr_96
.type nux_isr_96,@function
nux_isr_96:
    pushq $0
    pushq $96
    jmp nux_isr_common
.size nux_isr_96, .-nux_isr_96
.global nux_isr_97
.type nux_isr_97,@function
nux_isr_97:
    pushq $0
    pushq $97
    jmp nux_isr_common
.size nux_isr_97, .-nux_isr_97
.global nux_isr_98
.type nux_isr_98,@function
nux_isr_98:
    pushq $0
    pushq $98
    jmp nux_isr_common
.size nux_isr_98, .-nux_isr_98
.global nux_isr_99
.type nux_isr_99,@function
nux_isr_99:
    pushq $0
    pushq $99
    jmp nux_isr_common
.size nux_isr_99, .-nux_isr_99
.global nux_isr_100
.type nux_isr_100,@function
nux_isr_100:
    pushq $0
    pushq $100
    jmp nux_isr_common
.size nux_isr_100, .-nux_isr_100
.global nux_isr_101
.type nux_isr_101,@function
nux_isr_101:
    pushq $0
    pushq $101
    jmp nux_isr_common
.size nux_isr_101, .-nux_isr_101
.global nux_isr_102
.type nux_isr_102,@function
nux_isr_102:
    pushq $0
    pushq $102
    jmp nux_isr_common
.size nux_isr_102, .-nux_isr_102
.global nux_isr_103
.type nux_isr_103,@function
nux_isr_103:
    pushq $0
    pushq $103
    jmp nux_isr_common
.size nux_isr_103, .-nux_isr_103
.global nux_isr_104
.type nux_isr_104,@function
nux_isr_104:
    pushq $0
    pushq $104
    jmp nux_isr_common
.size nux_isr_104, .-nux_isr_104
.global nux_isr_105
.type nux_isr_105,@function
nux_isr_105:
    pushq $0
    pushq $105
    jmp nux_isr_common
.size nux_isr_105, .-nux_isr_105
.global nux_isr_106
.type nux_isr_106,@function
nux_isr_106:
    pushq $0
    pushq $106
    jmp nux_isr_common
.size nux_isr_106, .-nux_isr_106
.global nux_isr_107
.type nux_isr_107,@function
nux_isr_107:
    pushq $0
    pushq $107
    jmp nux_isr_common
.size nux_isr_107, .-nux_isr_107
.global nux_isr_108
.type nux_isr_108,@function
nux_isr_108:
    pushq $0
    pushq $108
    jmp nux_isr_common
.size nux_isr_108, .-nux_isr_108
.global nux_isr_109
.type nux_isr_109,@function
nux_isr_109:
    pushq $0
    pushq $109
    jmp nux_isr_common
.size nux_isr_109, .-nux_isr_109
.global nux_isr_110
.type nux_isr_110,@function
nux_isr_110:
    pushq $0
    pushq $110
    jmp nux_isr_common
.size nux_isr_110, .-nux_isr_110
.global nux_isr_111
.type nux_isr_111,@function
nux_isr_111:
    pushq $0
    pushq $111
    jmp nux_isr_common
.size nux_isr_111, .-nux_isr_111
.global nux_isr_112
.type nux_isr_112,@function
nux_isr_112:
    pushq $0
    pushq $112
    jmp nux_isr_common
.size nux_isr_112, .-nux_isr_112
.global nux_isr_113
.type nux_isr_113,@function
nux_isr_113:
    pushq $0
    pushq $113
    jmp nux_isr_common
.size nux_isr_113, .-nux_isr_113
.global nux_isr_114
.type nux_isr_114,@function
nux_isr_114:
    pushq $0
    pushq $114
    jmp nux_isr_common
.size nux_isr_114, .-nux_isr_114
.global nux_isr_115
.type nux_isr_115,@function
nux_isr_115:
    pushq $0
    pushq $115
    jmp nux_isr_common
.size nux_isr_115, .-nux_isr_115
.global nux_isr_116
.type nux_isr_116,@function
nux_isr_116:
    pushq $0
    pushq $116
    jmp nux_isr_common
.size nux_isr_116, .-nux_isr_116
.global nux_isr_117
.type nux_isr_117,@function
nux_isr_117:
    pushq $0
    pushq $117
    jmp nux_isr_common
.size nux_isr_117, .-nux_isr_117
.global nux_isr_118
.type nux_isr_118,@function
nux_isr_118:
    pushq $0
    pushq $118
    jmp nux_isr_common
.size nux_isr_118, .-nux_isr_118
.global nux_isr_119
.type nux_isr_119,@function
nux_isr_119:
    pushq $0
    pushq $119
    jmp nux_isr_common
.size nux_isr_119, .-nux_isr_119
.global nux_isr_120
.type nux_isr_120,@function
nux_isr_120:
    pushq $0
    pushq $120
    jmp nux_isr_common
.size nux_isr_120, .-nux_isr_120
.global nux_isr_121
.type nux_isr_121,@function
nux_isr_121:
    pushq $0
    pushq $121
    jmp nux_isr_common
.size nux_isr_121, .-nux_isr_121
.global nux_isr_122
.type nux_isr_122,@function
nux_isr_122:
    pushq $0
    pushq $122
    jmp nux_isr_common
.size nux_isr_122, .-nux_isr_122
.global nux_isr_123
.type nux_isr_123,@function
nux_isr_123:
    pushq $0
    pushq $123
    jmp nux_isr_common
.size nux_isr_123, .-nux_isr_123
.global nux_isr_124
.type nux_isr_124,@function
nux_isr_124:
    pushq $0
    pushq $124
    jmp nux_isr_common
.size nux_isr_124, .-nux_isr_124
.global nux_isr_125
.type nux_isr_125,@function
nux_isr_125:
    pushq $0
    pushq $125
    jmp nux_isr_common
.size nux_isr_125, .-nux_isr_125
.global nux_isr_126
.type nux_isr_126,@function
nux_isr_126:
    pushq $0
    pushq $126
    jmp nux_isr_common
.size nux_isr_126, .-nux_isr_126
.global nux_isr_127
.type nux_isr_127,@function
nux_isr_127:
    pushq $0
    pushq $127
    jmp nux_isr_common
.size nux_isr_127, .-nux_isr_127
.global nux_isr_128
.type nux_isr_128,@function
nux_isr_128:
    pushq $0
    pushq $128
    jmp nux_syscall_common
.size nux_isr_128, .-nux_isr_128

.type nux_syscall_common,@function
nux_syscall_common:
    pushq %rax
    pushq %rcx
    pushq %rdx
    pushq %rbx
    pushq %rbp
    pushq %rsi
    pushq %rdi
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
    subq $48, %rsp
    movq 112+48(%rsp), %rdi
    movq 64+48(%rsp), %rax
    movq %rax, 0(%rsp)
    movq 72+48(%rsp), %rax
    movq %rax, 8(%rsp)
    movq 96+48(%rsp), %rax
    movq %rax, 16(%rsp)
    movq 40+48(%rsp), %rax
    movq %rax, 24(%rsp)
    movq 56+48(%rsp), %rax
    movq %rax, 32(%rsp)
    movq 48+48(%rsp), %rax
    movq %rax, 40(%rsp)
    movq %rsp, %rsi
    call nux_syscall_invoke
    movq %rax, 112+48(%rsp)
    addq $48, %rsp
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %r11
    popq %r10
    popq %r9
    popq %r8
    popq %rdi
    popq %rsi
    popq %rbp
    popq %rbx
    popq %rdx
    popq %rcx
    popq %rax
    addq $16, %rsp
    iretq
.global nux_isr_129
.type nux_isr_129,@function
nux_isr_129:
    pushq $0
    pushq $129
    jmp nux_isr_common
.size nux_isr_129, .-nux_isr_129
.global nux_isr_130
.type nux_isr_130,@function
nux_isr_130:
    pushq $0
    pushq $130
    jmp nux_isr_common
.size nux_isr_130, .-nux_isr_130
.global nux_isr_131
.type nux_isr_131,@function
nux_isr_131:
    pushq $0
    pushq $131
    jmp nux_isr_common
.size nux_isr_131, .-nux_isr_131
.global nux_isr_132
.type nux_isr_132,@function
nux_isr_132:
    pushq $0
    pushq $132
    jmp nux_isr_common
.size nux_isr_132, .-nux_isr_132
.global nux_isr_133
.type nux_isr_133,@function
nux_isr_133:
    pushq $0
    pushq $133
    jmp nux_isr_common
.size nux_isr_133, .-nux_isr_133
.global nux_isr_134
.type nux_isr_134,@function
nux_isr_134:
    pushq $0
    pushq $134
    jmp nux_isr_common
.size nux_isr_134, .-nux_isr_134
.global nux_isr_135
.type nux_isr_135,@function
nux_isr_135:
    pushq $0
    pushq $135
    jmp nux_isr_common
.size nux_isr_135, .-nux_isr_135
.global nux_isr_136
.type nux_isr_136,@function
nux_isr_136:
    pushq $0
    pushq $136
    jmp nux_isr_common
.size nux_isr_136, .-nux_isr_136
.global nux_isr_137
.type nux_isr_137,@function
nux_isr_137:
    pushq $0
    pushq $137
    jmp nux_isr_common
.size nux_isr_137, .-nux_isr_137
.global nux_isr_138
.type nux_isr_138,@function
nux_isr_138:
    pushq $0
    pushq $138
    jmp nux_isr_common
.size nux_isr_138, .-nux_isr_138
.global nux_isr_139
.type nux_isr_139,@function
nux_isr_139:
    pushq $0
    pushq $139
    jmp nux_isr_common
.size nux_isr_139, .-nux_isr_139
.global nux_isr_140
.type nux_isr_140,@function
nux_isr_140:
    pushq $0
    pushq $140
    jmp nux_isr_common
.size nux_isr_140, .-nux_isr_140
.global nux_isr_141
.type nux_isr_141,@function
nux_isr_141:
    pushq $0
    pushq $141
    jmp nux_isr_common
.size nux_isr_141, .-nux_isr_141
.global nux_isr_142
.type nux_isr_142,@function
nux_isr_142:
    pushq $0
    pushq $142
    jmp nux_isr_common
.size nux_isr_142, .-nux_isr_142
.global nux_isr_143
.type nux_isr_143,@function
nux_isr_143:
    pushq $0
    pushq $143
    jmp nux_isr_common
.size nux_isr_143, .-nux_isr_143
.global nux_isr_144
.type nux_isr_144,@function
nux_isr_144:
    pushq $0
    pushq $144
    jmp nux_isr_common
.size nux_isr_144, .-nux_isr_144
.global nux_isr_145
.type nux_isr_145,@function
nux_isr_145:
    pushq $0
    pushq $145
    jmp nux_isr_common
.size nux_isr_145, .-nux_isr_145
.global nux_isr_146
.type nux_isr_146,@function
nux_isr_146:
    pushq $0
    pushq $146
    jmp nux_isr_common
.size nux_isr_146, .-nux_isr_146
.global nux_isr_147
.type nux_isr_147,@function
nux_isr_147:
    pushq $0
    pushq $147
    jmp nux_isr_common
.size nux_isr_147, .-nux_isr_147
.global nux_isr_148
.type nux_isr_148,@function
nux_isr_148:
    pushq $0
    pushq $148
    jmp nux_isr_common
.size nux_isr_148, .-nux_isr_148
.global nux_isr_149
.type nux_isr_149,@function
nux_isr_149:
    pushq $0
    pushq $149
    jmp nux_isr_common
.size nux_isr_149, .-nux_isr_149
.global nux_isr_150
.type nux_isr_150,@function
nux_isr_150:
    pushq $0
    pushq $150
    jmp nux_isr_common
.size nux_isr_150, .-nux_isr_150
.global nux_isr_151
.type nux_isr_151,@function
nux_isr_151:
    pushq $0
    pushq $151
    jmp nux_isr_common
.size nux_isr_151, .-nux_isr_151
.global nux_isr_152
.type nux_isr_152,@function
nux_isr_152:
    pushq $0
    pushq $152
    jmp nux_isr_common
.size nux_isr_152, .-nux_isr_152
.global nux_isr_153
.type nux_isr_153,@function
nux_isr_153:
    pushq $0
    pushq $153
    jmp nux_isr_common
.size nux_isr_153, .-nux_isr_153
.global nux_isr_154
.type nux_isr_154,@function
nux_isr_154:
    pushq $0
    pushq $154
    jmp nux_isr_common
.size nux_isr_154, .-nux_isr_154
.global nux_isr_155
.type nux_isr_155,@function
nux_isr_155:
    pushq $0
    pushq $155
    jmp nux_isr_common
.size nux_isr_155, .-nux_isr_155
.global nux_isr_156
.type nux_isr_156,@function
nux_isr_156:
    pushq $0
    pushq $156
    jmp nux_isr_common
.size nux_isr_156, .-nux_isr_156
.global nux_isr_157
.type nux_isr_157,@function
nux_isr_157:
    pushq $0
    pushq $157
    jmp nux_isr_common
.size nux_isr_157, .-nux_isr_157
.global nux_isr_158
.type nux_isr_158,@function
nux_isr_158:
    pushq $0
    pushq $158
    jmp nux_isr_common
.size nux_isr_158, .-nux_isr_158
.global nux_isr_159
.type nux_isr_159,@function
nux_isr_159:
    pushq $0
    pushq $159
    jmp nux_isr_common
.size nux_isr_159, .-nux_isr_159
.global nux_isr_160
.type nux_isr_160,@function
nux_isr_160:
    pushq $0
    pushq $160
    jmp nux_isr_common
.size nux_isr_160, .-nux_isr_160
.global nux_isr_161
.type nux_isr_161,@function
nux_isr_161:
    pushq $0
    pushq $161
    jmp nux_isr_common
.size nux_isr_161, .-nux_isr_161
.global nux_isr_162
.type nux_isr_162,@function
nux_isr_162:
    pushq $0
    pushq $162
    jmp nux_isr_common
.size nux_isr_162, .-nux_isr_162
.global nux_isr_163
.type nux_isr_163,@function
nux_isr_163:
    pushq $0
    pushq $163
    jmp nux_isr_common
.size nux_isr_163, .-nux_isr_163
.global nux_isr_164
.type nux_isr_164,@function
nux_isr_164:
    pushq $0
    pushq $164
    jmp nux_isr_common
.size nux_isr_164, .-nux_isr_164
.global nux_isr_165
.type nux_isr_165,@function
nux_isr_165:
    pushq $0
    pushq $165
    jmp nux_isr_common
.size nux_isr_165, .-nux_isr_165
.global nux_isr_166
.type nux_isr_166,@function
nux_isr_166:
    pushq $0
    pushq $166
    jmp nux_isr_common
.size nux_isr_166, .-nux_isr_166
.global nux_isr_167
.type nux_isr_167,@function
nux_isr_167:
    pushq $0
    pushq $167
    jmp nux_isr_common
.size nux_isr_167, .-nux_isr_167
.global nux_isr_168
.type nux_isr_168,@function
nux_isr_168:
    pushq $0
    pushq $168
    jmp nux_isr_common
.size nux_isr_168, .-nux_isr_168
.global nux_isr_169
.type nux_isr_169,@function
nux_isr_169:
    pushq $0
    pushq $169
    jmp nux_isr_common
.size nux_isr_169, .-nux_isr_169
.global nux_isr_170
.type nux_isr_170,@function
nux_isr_170:
    pushq $0
    pushq $170
    jmp nux_isr_common
.size nux_isr_170, .-nux_isr_170
.global nux_isr_171
.type nux_isr_171,@function
nux_isr_171:
    pushq $0
    pushq $171
    jmp nux_isr_common
.size nux_isr_171, .-nux_isr_171
.global nux_isr_172
.type nux_isr_172,@function
nux_isr_172:
    pushq $0
    pushq $172
    jmp nux_isr_common
.size nux_isr_172, .-nux_isr_172
.global nux_isr_173
.type nux_isr_173,@function
nux_isr_173:
    pushq $0
    pushq $173
    jmp nux_isr_common
.size nux_isr_173, .-nux_isr_173
.global nux_isr_174
.type nux_isr_174,@function
nux_isr_174:
    pushq $0
    pushq $174
    jmp nux_isr_common
.size nux_isr_174, .-nux_isr_174
.global nux_isr_175
.type nux_isr_175,@function
nux_isr_175:
    pushq $0
    pushq $175
    jmp nux_isr_common
.size nux_isr_175, .-nux_isr_175
.global nux_isr_176
.type nux_isr_176,@function
nux_isr_176:
    pushq $0
    pushq $176
    jmp nux_isr_common
.size nux_isr_176, .-nux_isr_176
.global nux_isr_177
.type nux_isr_177,@function
nux_isr_177:
    pushq $0
    pushq $177
    jmp nux_isr_common
.size nux_isr_177, .-nux_isr_177
.global nux_isr_178
.type nux_isr_178,@function
nux_isr_178:
    pushq $0
    pushq $178
    jmp nux_isr_common
.size nux_isr_178, .-nux_isr_178
.global nux_isr_179
.type nux_isr_179,@function
nux_isr_179:
    pushq $0
    pushq $179
    jmp nux_isr_common
.size nux_isr_179, .-nux_isr_179
.global nux_isr_180
.type nux_isr_180,@function
nux_isr_180:
    pushq $0
    pushq $180
    jmp nux_isr_common
.size nux_isr_180, .-nux_isr_180
.global nux_isr_181
.type nux_isr_181,@function
nux_isr_181:
    pushq $0
    pushq $181
    jmp nux_isr_common
.size nux_isr_181, .-nux_isr_181
.global nux_isr_182
.type nux_isr_182,@function
nux_isr_182:
    pushq $0
    pushq $182
    jmp nux_isr_common
.size nux_isr_182, .-nux_isr_182
.global nux_isr_183
.type nux_isr_183,@function
nux_isr_183:
    pushq $0
    pushq $183
    jmp nux_isr_common
.size nux_isr_183, .-nux_isr_183
.global nux_isr_184
.type nux_isr_184,@function
nux_isr_184:
    pushq $0
    pushq $184
    jmp nux_isr_common
.size nux_isr_184, .-nux_isr_184
.global nux_isr_185
.type nux_isr_185,@function
nux_isr_185:
    pushq $0
    pushq $185
    jmp nux_isr_common
.size nux_isr_185, .-nux_isr_185
.global nux_isr_186
.type nux_isr_186,@function
nux_isr_186:
    pushq $0
    pushq $186
    jmp nux_isr_common
.size nux_isr_186, .-nux_isr_186
.global nux_isr_187
.type nux_isr_187,@function
nux_isr_187:
    pushq $0
    pushq $187
    jmp nux_isr_common
.size nux_isr_187, .-nux_isr_187
.global nux_isr_188
.type nux_isr_188,@function
nux_isr_188:
    pushq $0
    pushq $188
    jmp nux_isr_common
.size nux_isr_188, .-nux_isr_188
.global nux_isr_189
.type nux_isr_189,@function
nux_isr_189:
    pushq $0
    pushq $189
    jmp nux_isr_common
.size nux_isr_189, .-nux_isr_189
.global nux_isr_190
.type nux_isr_190,@function
nux_isr_190:
    pushq $0
    pushq $190
    jmp nux_isr_common
.size nux_isr_190, .-nux_isr_190
.global nux_isr_191
.type nux_isr_191,@function
nux_isr_191:
    pushq $0
    pushq $191
    jmp nux_isr_common
.size nux_isr_191, .-nux_isr_191
.global nux_isr_192
.type nux_isr_192,@function
nux_isr_192:
    pushq $0
    pushq $192
    jmp nux_isr_common
.size nux_isr_192, .-nux_isr_192
.global nux_isr_193
.type nux_isr_193,@function
nux_isr_193:
    pushq $0
    pushq $193
    jmp nux_isr_common
.size nux_isr_193, .-nux_isr_193
.global nux_isr_194
.type nux_isr_194,@function
nux_isr_194:
    pushq $0
    pushq $194
    jmp nux_isr_common
.size nux_isr_194, .-nux_isr_194
.global nux_isr_195
.type nux_isr_195,@function
nux_isr_195:
    pushq $0
    pushq $195
    jmp nux_isr_common
.size nux_isr_195, .-nux_isr_195
.global nux_isr_196
.type nux_isr_196,@function
nux_isr_196:
    pushq $0
    pushq $196
    jmp nux_isr_common
.size nux_isr_196, .-nux_isr_196
.global nux_isr_197
.type nux_isr_197,@function
nux_isr_197:
    pushq $0
    pushq $197
    jmp nux_isr_common
.size nux_isr_197, .-nux_isr_197
.global nux_isr_198
.type nux_isr_198,@function
nux_isr_198:
    pushq $0
    pushq $198
    jmp nux_isr_common
.size nux_isr_198, .-nux_isr_198
.global nux_isr_199
.type nux_isr_199,@function
nux_isr_199:
    pushq $0
    pushq $199
    jmp nux_isr_common
.size nux_isr_199, .-nux_isr_199
.global nux_isr_200
.type nux_isr_200,@function
nux_isr_200:
    pushq $0
    pushq $200
    jmp nux_isr_common
.size nux_isr_200, .-nux_isr_200
.global nux_isr_201
.type nux_isr_201,@function
nux_isr_201:
    pushq $0
    pushq $201
    jmp nux_isr_common
.size nux_isr_201, .-nux_isr_201
.global nux_isr_202
.type nux_isr_202,@function
nux_isr_202:
    pushq $0
    pushq $202
    jmp nux_isr_common
.size nux_isr_202, .-nux_isr_202
.global nux_isr_203
.type nux_isr_203,@function
nux_isr_203:
    pushq $0
    pushq $203
    jmp nux_isr_common
.size nux_isr_203, .-nux_isr_203
.global nux_isr_204
.type nux_isr_204,@function
nux_isr_204:
    pushq $0
    pushq $204
    jmp nux_isr_common
.size nux_isr_204, .-nux_isr_204
.global nux_isr_205
.type nux_isr_205,@function
nux_isr_205:
    pushq $0
    pushq $205
    jmp nux_isr_common
.size nux_isr_205, .-nux_isr_205
.global nux_isr_206
.type nux_isr_206,@function
nux_isr_206:
    pushq $0
    pushq $206
    jmp nux_isr_common
.size nux_isr_206, .-nux_isr_206
.global nux_isr_207
.type nux_isr_207,@function
nux_isr_207:
    pushq $0
    pushq $207
    jmp nux_isr_common
.size nux_isr_207, .-nux_isr_207
.global nux_isr_208
.type nux_isr_208,@function
nux_isr_208:
    pushq $0
    pushq $208
    jmp nux_isr_common
.size nux_isr_208, .-nux_isr_208
.global nux_isr_209
.type nux_isr_209,@function
nux_isr_209:
    pushq $0
    pushq $209
    jmp nux_isr_common
.size nux_isr_209, .-nux_isr_209
.global nux_isr_210
.type nux_isr_210,@function
nux_isr_210:
    pushq $0
    pushq $210
    jmp nux_isr_common
.size nux_isr_210, .-nux_isr_210
.global nux_isr_211
.type nux_isr_211,@function
nux_isr_211:
    pushq $0
    pushq $211
    jmp nux_isr_common
.size nux_isr_211, .-nux_isr_211
.global nux_isr_212
.type nux_isr_212,@function
nux_isr_212:
    pushq $0
    pushq $212
    jmp nux_isr_common
.size nux_isr_212, .-nux_isr_212
.global nux_isr_213
.type nux_isr_213,@function
nux_isr_213:
    pushq $0
    pushq $213
    jmp nux_isr_common
.size nux_isr_213, .-nux_isr_213
.global nux_isr_214
.type nux_isr_214,@function
nux_isr_214:
    pushq $0
    pushq $214
    jmp nux_isr_common
.size nux_isr_214, .-nux_isr_214
.global nux_isr_215
.type nux_isr_215,@function
nux_isr_215:
    pushq $0
    pushq $215
    jmp nux_isr_common
.size nux_isr_215, .-nux_isr_215
.global nux_isr_216
.type nux_isr_216,@function
nux_isr_216:
    pushq $0
    pushq $216
    jmp nux_isr_common
.size nux_isr_216, .-nux_isr_216
.global nux_isr_217
.type nux_isr_217,@function
nux_isr_217:
    pushq $0
    pushq $217
    jmp nux_isr_common
.size nux_isr_217, .-nux_isr_217
.global nux_isr_218
.type nux_isr_218,@function
nux_isr_218:
    pushq $0
    pushq $218
    jmp nux_isr_common
.size nux_isr_218, .-nux_isr_218
.global nux_isr_219
.type nux_isr_219,@function
nux_isr_219:
    pushq $0
    pushq $219
    jmp nux_isr_common
.size nux_isr_219, .-nux_isr_219
.global nux_isr_220
.type nux_isr_220,@function
nux_isr_220:
    pushq $0
    pushq $220
    jmp nux_isr_common
.size nux_isr_220, .-nux_isr_220
.global nux_isr_221
.type nux_isr_221,@function
nux_isr_221:
    pushq $0
    pushq $221
    jmp nux_isr_common
.size nux_isr_221, .-nux_isr_221
.global nux_isr_222
.type nux_isr_222,@function
nux_isr_222:
    pushq $0
    pushq $222
    jmp nux_isr_common
.size nux_isr_222, .-nux_isr_222
.global nux_isr_223
.type nux_isr_223,@function
nux_isr_223:
    pushq $0
    pushq $223
    jmp nux_isr_common
.size nux_isr_223, .-nux_isr_223
.global nux_isr_224
.type nux_isr_224,@function
nux_isr_224:
    pushq $0
    pushq $224
    jmp nux_isr_common
.size nux_isr_224, .-nux_isr_224
.global nux_isr_225
.type nux_isr_225,@function
nux_isr_225:
    pushq $0
    pushq $225
    jmp nux_isr_common
.size nux_isr_225, .-nux_isr_225
.global nux_isr_226
.type nux_isr_226,@function
nux_isr_226:
    pushq $0
    pushq $226
    jmp nux_isr_common
.size nux_isr_226, .-nux_isr_226
.global nux_isr_227
.type nux_isr_227,@function
nux_isr_227:
    pushq $0
    pushq $227
    jmp nux_isr_common
.size nux_isr_227, .-nux_isr_227
.global nux_isr_228
.type nux_isr_228,@function
nux_isr_228:
    pushq $0
    pushq $228
    jmp nux_isr_common
.size nux_isr_228, .-nux_isr_228
.global nux_isr_229
.type nux_isr_229,@function
nux_isr_229:
    pushq $0
    pushq $229
    jmp nux_isr_common
.size nux_isr_229, .-nux_isr_229
.global nux_isr_230
.type nux_isr_230,@function
nux_isr_230:
    pushq $0
    pushq $230
    jmp nux_isr_common
.size nux_isr_230, .-nux_isr_230
.global nux_isr_231
.type nux_isr_231,@function
nux_isr_231:
    pushq $0
    pushq $231
    jmp nux_isr_common
.size nux_isr_231, .-nux_isr_231
.global nux_isr_232
.type nux_isr_232,@function
nux_isr_232:
    pushq $0
    pushq $232
    jmp nux_isr_common
.size nux_isr_232, .-nux_isr_232
.global nux_isr_233
.type nux_isr_233,@function
nux_isr_233:
    pushq $0
    pushq $233
    jmp nux_isr_common
.size nux_isr_233, .-nux_isr_233
.global nux_isr_234
.type nux_isr_234,@function
nux_isr_234:
    pushq $0
    pushq $234
    jmp nux_isr_common
.size nux_isr_234, .-nux_isr_234
.global nux_isr_235
.type nux_isr_235,@function
nux_isr_235:
    pushq $0
    pushq $235
    jmp nux_isr_common
.size nux_isr_235, .-nux_isr_235
.global nux_isr_236
.type nux_isr_236,@function
nux_isr_236:
    pushq $0
    pushq $236
    jmp nux_isr_common
.size nux_isr_236, .-nux_isr_236
.global nux_isr_237
.type nux_isr_237,@function
nux_isr_237:
    pushq $0
    pushq $237
    jmp nux_isr_common
.size nux_isr_237, .-nux_isr_237
.global nux_isr_238
.type nux_isr_238,@function
nux_isr_238:
    pushq $0
    pushq $238
    jmp nux_isr_common
.size nux_isr_238, .-nux_isr_238
.global nux_isr_239
.type nux_isr_239,@function
nux_isr_239:
    pushq $0
    pushq $239
    jmp nux_isr_common
.size nux_isr_239, .-nux_isr_239
.global nux_isr_240
.type nux_isr_240,@function
nux_isr_240:
    pushq $0
    pushq $240
    jmp nux_isr_common
.size nux_isr_240, .-nux_isr_240
.global nux_isr_241
.type nux_isr_241,@function
nux_isr_241:
    pushq $0
    pushq $241
    jmp nux_isr_common
.size nux_isr_241, .-nux_isr_241
.global nux_isr_242
.type nux_isr_242,@function
nux_isr_242:
    pushq $0
    pushq $242
    jmp nux_isr_common
.size nux_isr_242, .-nux_isr_242
.global nux_isr_243
.type nux_isr_243,@function
nux_isr_243:
    pushq $0
    pushq $243
    jmp nux_isr_common
.size nux_isr_243, .-nux_isr_243
.global nux_isr_244
.type nux_isr_244,@function
nux_isr_244:
    pushq $0
    pushq $244
    jmp nux_isr_common
.size nux_isr_244, .-nux_isr_244
.global nux_isr_245
.type nux_isr_245,@function
nux_isr_245:
    pushq $0
    pushq $245
    jmp nux_isr_common
.size nux_isr_245, .-nux_isr_245
.global nux_isr_246
.type nux_isr_246,@function
nux_isr_246:
    pushq $0
    pushq $246
    jmp nux_isr_common
.size nux_isr_246, .-nux_isr_246
.global nux_isr_247
.type nux_isr_247,@function
nux_isr_247:
    pushq $0
    pushq $247
    jmp nux_isr_common
.size nux_isr_247, .-nux_isr_247
.global nux_isr_248
.type nux_isr_248,@function
nux_isr_248:
    pushq $0
    pushq $248
    jmp nux_isr_common
.size nux_isr_248, .-nux_isr_248
.global nux_isr_249
.type nux_isr_249,@function
nux_isr_249:
    pushq $0
    pushq $249
    jmp nux_isr_common
.size nux_isr_249, .-nux_isr_249
.global nux_isr_250
.type nux_isr_250,@function
nux_isr_250:
    pushq $0
    pushq $250
    jmp nux_isr_common
.size nux_isr_250, .-nux_isr_250
.global nux_isr_251
.type nux_isr_251,@function
nux_isr_251:
    pushq $0
    pushq $251
    jmp nux_isr_common
.size nux_isr_251, .-nux_isr_251
.global nux_isr_252
.type nux_isr_252,@function
nux_isr_252:
    pushq $0
    pushq $252
    jmp nux_isr_common
.size nux_isr_252, .-nux_isr_252
.global nux_isr_253
.type nux_isr_253,@function
nux_isr_253:
    pushq $0
    pushq $253
    jmp nux_isr_common
.size nux_isr_253, .-nux_isr_253
.global nux_isr_254
.type nux_isr_254,@function
nux_isr_254:
    pushq $0
    pushq $254
    jmp nux_isr_common
.size nux_isr_254, .-nux_isr_254
.global nux_isr_255
.type nux_isr_255,@function
nux_isr_255:
    pushq $0
    pushq $255
    jmp nux_isr_common
.size nux_isr_255, .-nux_isr_255

.type nux_isr_common,@function
nux_isr_common:
    pushq %rax
    pushq %rcx
    pushq %rdx
    pushq %rbx
    pushq %rbp
    pushq %rsi
    pushq %rdi
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
    movq %rsp, %r12
    movq 120(%rsp), %rdi
    movq 128(%rsp), %rsi
    movq %cr2, %rdx
    movq %r12, %rcx
    andq $-16, %rsp
    call nux_arch_exception_dispatch
    movq %r12, %rsp
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %r11
    popq %r10
    popq %r9
    popq %r8
    popq %rdi
    popq %rsi
    popq %rbp
    popq %rbx
    popq %rdx
    popq %rcx
    popq %rax
    addq $16, %rsp
    iretq

.section .rodata
.align 8
.global nux_isr_stub_table
.type nux_isr_stub_table,@object
nux_isr_stub_table:
    .quad nux_isr_0
    .quad nux_isr_1
    .quad nux_isr_2
    .quad nux_isr_3
    .quad nux_isr_4
    .quad nux_isr_5
    .quad nux_isr_6
    .quad nux_isr_7
    .quad nux_isr_8
    .quad nux_isr_9
    .quad nux_isr_10
    .quad nux_isr_11
    .quad nux_isr_12
    .quad nux_isr_13
    .quad nux_isr_14
    .quad nux_isr_15
    .quad nux_isr_16
    .quad nux_isr_17
    .quad nux_isr_18
    .quad nux_isr_19
    .quad nux_isr_20
    .quad nux_isr_21
    .quad nux_isr_22
    .quad nux_isr_23
    .quad nux_isr_24
    .quad nux_isr_25
    .quad nux_isr_26
    .quad nux_isr_27
    .quad nux_isr_28
    .quad nux_isr_29
    .quad nux_isr_30
    .quad nux_isr_31
    .quad nux_isr_32
    .quad nux_isr_33
    .quad nux_isr_34
    .quad nux_isr_35
    .quad nux_isr_36
    .quad nux_isr_37
    .quad nux_isr_38
    .quad nux_isr_39
    .quad nux_isr_40
    .quad nux_isr_41
    .quad nux_isr_42
    .quad nux_isr_43
    .quad nux_isr_44
    .quad nux_isr_45
    .quad nux_isr_46
    .quad nux_isr_47
    .quad nux_isr_48
    .quad nux_isr_49
    .quad nux_isr_50
    .quad nux_isr_51
    .quad nux_isr_52
    .quad nux_isr_53
    .quad nux_isr_54
    .quad nux_isr_55
    .quad nux_isr_56
    .quad nux_isr_57
    .quad nux_isr_58
    .quad nux_isr_59
    .quad nux_isr_60
    .quad nux_isr_61
    .quad nux_isr_62
    .quad nux_isr_63
    .quad nux_isr_64
    .quad nux_isr_65
    .quad nux_isr_66
    .quad nux_isr_67
    .quad nux_isr_68
    .quad nux_isr_69
    .quad nux_isr_70
    .quad nux_isr_71
    .quad nux_isr_72
    .quad nux_isr_73
    .quad nux_isr_74
    .quad nux_isr_75
    .quad nux_isr_76
    .quad nux_isr_77
    .quad nux_isr_78
    .quad nux_isr_79
    .quad nux_isr_80
    .quad nux_isr_81
    .quad nux_isr_82
    .quad nux_isr_83
    .quad nux_isr_84
    .quad nux_isr_85
    .quad nux_isr_86
    .quad nux_isr_87
    .quad nux_isr_88
    .quad nux_isr_89
    .quad nux_isr_90
    .quad nux_isr_91
    .quad nux_isr_92
    .quad nux_isr_93
    .quad nux_isr_94
    .quad nux_isr_95
    .quad nux_isr_96
    .quad nux_isr_97
    .quad nux_isr_98
    .quad nux_isr_99
    .quad nux_isr_100
    .quad nux_isr_101
    .quad nux_isr_102
    .quad nux_isr_103
    .quad nux_isr_104
    .quad nux_isr_105
    .quad nux_isr_106
    .quad nux_isr_107
    .quad nux_isr_108
    .quad nux_isr_109
    .quad nux_isr_110
    .quad nux_isr_111
    .quad nux_isr_112
    .quad nux_isr_113
    .quad nux_isr_114
    .quad nux_isr_115
    .quad nux_isr_116
    .quad nux_isr_117
    .quad nux_isr_118
    .quad nux_isr_119
    .quad nux_isr_120
    .quad nux_isr_121
    .quad nux_isr_122
    .quad nux_isr_123
    .quad nux_isr_124
    .quad nux_isr_125
    .quad nux_isr_126
    .quad nux_isr_127
    .quad nux_isr_128
    .quad nux_isr_129
    .quad nux_isr_130
    .quad nux_isr_131
    .quad nux_isr_132
    .quad nux_isr_133
    .quad nux_isr_134
    .quad nux_isr_135
    .quad nux_isr_136
    .quad nux_isr_137
    .quad nux_isr_138
    .quad nux_isr_139
    .quad nux_isr_140
    .quad nux_isr_141
    .quad nux_isr_142
    .quad nux_isr_143
    .quad nux_isr_144
    .quad nux_isr_145
    .quad nux_isr_146
    .quad nux_isr_147
    .quad nux_isr_148
    .quad nux_isr_149
    .quad nux_isr_150
    .quad nux_isr_151
    .quad nux_isr_152
    .quad nux_isr_153
    .quad nux_isr_154
    .quad nux_isr_155
    .quad nux_isr_156
    .quad nux_isr_157
    .quad nux_isr_158
    .quad nux_isr_159
    .quad nux_isr_160
    .quad nux_isr_161
    .quad nux_isr_162
    .quad nux_isr_163
    .quad nux_isr_164
    .quad nux_isr_165
    .quad nux_isr_166
    .quad nux_isr_167
    .quad nux_isr_168
    .quad nux_isr_169
    .quad nux_isr_170
    .quad nux_isr_171
    .quad nux_isr_172
    .quad nux_isr_173
    .quad nux_isr_174
    .quad nux_isr_175
    .quad nux_isr_176
    .quad nux_isr_177
    .quad nux_isr_178
    .quad nux_isr_179
    .quad nux_isr_180
    .quad nux_isr_181
    .quad nux_isr_182
    .quad nux_isr_183
    .quad nux_isr_184
    .quad nux_isr_185
    .quad nux_isr_186
    .quad nux_isr_187
    .quad nux_isr_188
    .quad nux_isr_189
    .quad nux_isr_190
    .quad nux_isr_191
    .quad nux_isr_192
    .quad nux_isr_193
    .quad nux_isr_194
    .quad nux_isr_195
    .quad nux_isr_196
    .quad nux_isr_197
    .quad nux_isr_198
    .quad nux_isr_199
    .quad nux_isr_200
    .quad nux_isr_201
    .quad nux_isr_202
    .quad nux_isr_203
    .quad nux_isr_204
    .quad nux_isr_205
    .quad nux_isr_206
    .quad nux_isr_207
    .quad nux_isr_208
    .quad nux_isr_209
    .quad nux_isr_210
    .quad nux_isr_211
    .quad nux_isr_212
    .quad nux_isr_213
    .quad nux_isr_214
    .quad nux_isr_215
    .quad nux_isr_216
    .quad nux_isr_217
    .quad nux_isr_218
    .quad nux_isr_219
    .quad nux_isr_220
    .quad nux_isr_221
    .quad nux_isr_222
    .quad nux_isr_223
    .quad nux_isr_224
    .quad nux_isr_225
    .quad nux_isr_226
    .quad nux_isr_227
    .quad nux_isr_228
    .quad nux_isr_229
    .quad nux_isr_230
    .quad nux_isr_231
    .quad nux_isr_232
    .quad nux_isr_233
    .quad nux_isr_234
    .quad nux_isr_235
    .quad nux_isr_236
    .quad nux_isr_237
    .quad nux_isr_238
    .quad nux_isr_239
    .quad nux_isr_240
    .quad nux_isr_241
    .quad nux_isr_242
    .quad nux_isr_243
    .quad nux_isr_244
    .quad nux_isr_245
    .quad nux_isr_246
    .quad nux_isr_247
    .quad nux_isr_248
    .quad nux_isr_249
    .quad nux_isr_250
    .quad nux_isr_251
    .quad nux_isr_252
    .quad nux_isr_253
    .quad nux_isr_254
    .quad nux_isr_255
.size nux_isr_stub_table, .-nux_isr_stub_table
.section .note.GNU-stack,"",@progbits
