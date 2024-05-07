/*-------------------------------------------------------------------------
    This source file is a part of the MC6809 Simulator
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2024, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/
//
//  MC6809.cpp
//  6809 simulator
//
//  Created by Chris Marrin on 4/22/24.
//

#include <string>
#include <iostream>
#include <iomanip>

#include "MC6809.h"
#include "srec.h"

using namespace mc6809;

class SRecordInfo : public SRecordParser
{
  public:
    SRecordInfo(uint8_t* ram) : _ram(ram) { }
    virtual  ~SRecordInfo() { }
    
    uint16_t startAddr() const { return _startAddr; }

  protected:
    virtual bool Header(const SRecordHeader *sRecHdr)
    {
        return true;
    }
    
    virtual bool StartAddress(const SRecordData *sRecData)
    {
        _startAddr = sRecData->m_addr;
        return true;
    }
    
    virtual bool Data(const SRecordData *sRecData)
    {
        // FIXME: Need to handle ranges, which means we need to pass in the ram size
        
        // If the start addr has not been set, set it to the start of the first record.
        // The StartAddress function can change this at the end
        if (!_startAddrSet) {
            _startAddr = sRecData->m_addr;
        }
        memcpy(_ram + sRecData->m_addr, sRecData->m_data, sRecData->m_dataLen);
        return true;
    }
    
  private:
    uint8_t* _ram = nullptr;
    uint16_t _startAddr = 0;
    bool _startAddrSet = false;
};

static_assert (sizeof(Opcode) == 4, "Opcode is wrong size");

static constexpr Opcode opcodeTable[ ] = {
    /*00*/  	{ Op::NEG	  , Left::LdSt, Right::None , Adr::Direct	, Reg::M8   , 6  },
    /*01*/  	{ Op::ILL	  , Left::None, Right::None , Adr::None     , Reg::None , 0  },
    /*02*/  	{ Op::ILL	  , Left::None, Right::None , Adr::None     , Reg::None , 0  },
    /*03*/  	{ Op::COM	  , Left::LdSt, Right::None , Adr::Direct	, Reg::M8   , 6  },
    /*04*/  	{ Op::LSR	  , Left::LdSt, Right::None , Adr::Direct	, Reg::M8   , 6  },
    /*05*/  	{ Op::ILL	  , Left::None, Right::None , Adr::None     , Reg::None , 0  },
    /*06*/  	{ Op::ROR	  , Left::LdSt, Right::None , Adr::Direct	, Reg::M8   , 6	 },
    /*07*/  	{ Op::ASR	  , Left::LdSt, Right::None , Adr::Direct	, Reg::M8   , 6	 },
    /*08*/  	{ Op::ASL	  , Left::LdSt, Right::None , Adr::Direct	, Reg::M8   , 6	 },
    /*09*/  	{ Op::ROL	  , Left::LdSt, Right::None , Adr::Direct	, Reg::M8   , 6	 },
    /*0A*/  	{ Op::DEC	  , Left::LdSt, Right::None , Adr::Direct	, Reg::M8   , 6	 },
    /*0B*/  	{ Op::ILL	  , Left::None, Right::None , Adr::None     , Reg::None , 0  },
    /*0C*/  	{ Op::INC	  , Left::LdSt, Right::None , Adr::Direct	, Reg::M8   , 6	 },
    /*0D*/  	{ Op::TST	  , Left::Ld  , Right::None , Adr::Direct	, Reg::M8   , 6	 },
    /*0E*/  	{ Op::JMP	  , Left::None, Right::None , Adr::Direct	, Reg::None , 3	 },
    /*0F*/  	{ Op::CLR	  , Left::St  , Right::None  , Adr::Direct	, Reg::M8   , 6	 },
    /*10*/  	{ Op::Page2	  , Left::None, Right::None , Adr::None	    , Reg::None , 0	 },
    /*11*/  	{ Op::Page3	  , Left::None, Right::None , Adr::None	    , Reg::None , 0	 },
    /*12*/  	{ Op::NOP	  , Left::None, Right::None , Adr::Inherent , Reg::None , 2	 },
    /*13*/  	{ Op::SYNC	  , Left::None, Right::None , Adr::Inherent	, Reg::None , 4	 },
    /*14*/  	{ Op::ILL	  , Left::None, Right::None , Adr::None     , Reg::None , 0  },
    /*15*/  	{ Op::ILL	  , Left::None, Right::None , Adr::None     , Reg::None , 0  },
    /*16*/  	{ Op::BRA	  , Left::None, Right::None , Adr::RelL	    , Reg::None , 5	 },
    /*17*/  	{ Op::BSR	  , Left::None, Right::None , Adr::RelL	    , Reg::None , 9	 },
    /*18*/  	{ Op::ILL	  , Left::None, Right::None , Adr::None     , Reg::None , 0  },
    /*19*/  	{ Op::DAA	  , Left::None, Right::None , Adr::Inherent	, Reg::None , 2	 },
    /*1A*/  	{ Op::ORCC	  , Left::None, Right::None , Adr::Immed8	, Reg::None , 3	 },
    /*1B*/  	{ Op::ILL	  , Left::None, Right::None , Adr::None     , Reg::None , 0  },
    /*1C*/  	{ Op::ANDCC	  , Left::None, Right::None , Adr::Immed8	, Reg::None , 3	 },
    /*1D*/  	{ Op::SEX	  , Left::None, Right::None , Adr::Inherent	, Reg::None , 2	 },
    /*1E*/  	{ Op::EXG	  , Left::None, Right::None , Adr::Immed8	, Reg::None , 8	 },
    /*1F*/  	{ Op::TFR	  , Left::None, Right::None , Adr::Immed8	, Reg::None , 6	 },
    /*20*/  	{ Op::BRA	  , Left::None, Right::None , Adr::Rel	    , Reg::None , 3	 },
    /*21*/  	{ Op::BRN	  , Left::None, Right::None , Adr::RelP	    , Reg::None , 3	 },
    /*22*/  	{ Op::BHI	  , Left::None, Right::None , Adr::RelP	    , Reg::None , 3	 },
    /*23*/  	{ Op::BLS	  , Left::None, Right::None , Adr::RelP	    , Reg::None , 3	 },
    /*24*/  	{ Op::BHS	  , Left::None, Right::None , Adr::RelP	    , Reg::None , 3	 },
    /*25*/  	{ Op::BLO	  , Left::None, Right::None , Adr::RelP	    , Reg::None , 3	 },
    /*26*/  	{ Op::BNE	  , Left::None, Right::None , Adr::RelP	    , Reg::None , 3	 },
    /*27*/  	{ Op::BEQ	  , Left::None, Right::None , Adr::RelP	    , Reg::None , 3	 },
    /*28*/  	{ Op::BVC	  , Left::None, Right::None , Adr::RelP	    , Reg::None , 3	 },
    /*29*/  	{ Op::BVS	  , Left::None, Right::None , Adr::RelP	    , Reg::None , 3	 },
    /*2A*/  	{ Op::BPL	  , Left::None, Right::None , Adr::RelP	    , Reg::None , 3	 },
    /*2B*/  	{ Op::BMI	  , Left::None, Right::None , Adr::RelP	    , Reg::None , 3	 },
    /*2C*/  	{ Op::BGE	  , Left::None, Right::None , Adr::RelP	    , Reg::None , 3	 },
    /*2D*/  	{ Op::BLT	  , Left::None, Right::None , Adr::RelP	    , Reg::None , 3	 },
    /*2E*/  	{ Op::BGT	  , Left::None, Right::None , Adr::RelP	    , Reg::None , 3	 },
    /*2F*/  	{ Op::BLE	  , Left::None, Right::None , Adr::RelP	    , Reg::None , 3	 },
    /*30*/  	{ Op::LEA	  , Left::St  , Right::None , Adr::Indexed	, Reg::X    , 4	 },
    /*31*/  	{ Op::LEA	  , Left::None, Right::None , Adr::Indexed	, Reg::Y    , 4	 },
    /*32*/  	{ Op::LEA	  , Left::None, Right::None , Adr::Indexed	, Reg::S    , 4	 },
    /*33*/  	{ Op::LEA	  , Left::None, Right::None , Adr::Indexed	, Reg::U    , 4	 },
    /*34*/  	{ Op::PSH	  , Left::None, Right::None , Adr::Immed8	, Reg::S    , 5	 },
    /*35*/  	{ Op::PUL	  , Left::None, Right::None , Adr::Immed8	, Reg::S    , 5	 },
    /*36*/  	{ Op::PSH	  , Left::None, Right::None , Adr::Immed8	, Reg::U    , 5	 },
    /*37*/  	{ Op::PUL	  , Left::None, Right::None , Adr::Immed8	, Reg::U    , 5	 },
    /*38*/  	{ Op::ILL	  , Left::None, Right::None , Adr::None	    , Reg::None , 0	 },
    /*39*/  	{ Op::RTS	  , Left::None, Right::None , Adr::Inherent	, Reg::None , 5	 },
    /*3A*/  	{ Op::ABX	  , Left::None, Right::None , Adr::Inherent	, Reg::None , 3	 },
    /*3B*/  	{ Op::RTI	  , Left::None, Right::None , Adr::Inherent	, Reg::None , 6	 },          // 6 if FIRQ, 15 if IRQ
    /*3C*/  	{ Op::CWAI	  , Left::None, Right::None , Adr::Inherent	, Reg::None , 20 },
    /*3D*/  	{ Op::MUL	  , Left::None, Right::None , Adr::Inherent	, Reg::None , 11 },
    /*3E*/  	{ Op::ILL	  , Left::None, Right::None , Adr::None	    , Reg::None , 0	 },
    /*3F*/  	{ Op::SWI	  , Left::None, Right::None , Adr::Inherent	, Reg::None , 19 },
    /*40*/  	{ Op::NEG	  , Left::LdSt, Right::None , Adr::Inherent	, Reg::A    , 2	 },
    /*41*/  	{ Op::ILL	  , Left::None, Right::None , Adr::None	    , Reg::None , 0	 },
    /*42*/  	{ Op::ILL	  , Left::None, Right::None , Adr::None     , Reg::None , 0  },
    /*43*/  	{ Op::COM	  , Left::LdSt, Right::None , Adr::Inherent	, Reg::A    , 2	 },
    /*44*/  	{ Op::LSR	  , Left::LdSt, Right::None , Adr::Inherent	, Reg::A    , 2	 },
    /*45*/  	{ Op::ILL	  , Left::None, Right::None , Adr::None     , Reg::None , 0  },
    /*46*/  	{ Op::ROR	  , Left::LdSt, Right::None , Adr::Inherent	, Reg::A    , 2	 },
    /*47*/  	{ Op::ASR	  , Left::LdSt, Right::None , Adr::Inherent	, Reg::A    , 2	 },
    /*48*/  	{ Op::ASL	  , Left::LdSt, Right::None , Adr::Inherent	, Reg::A    , 2	 },
    /*49*/  	{ Op::ROL	  , Left::LdSt, Right::None , Adr::Inherent	, Reg::A    , 2	 },
    /*4A*/  	{ Op::DEC	  , Left::LdSt, Right::None , Adr::Inherent	, Reg::A    , 2	 },
    /*4B*/  	{ Op::ILL	  , Left::None, Right::None , Adr::None     , Reg::None , 0  },
    /*4C*/  	{ Op::INC	  , Left::LdSt, Right::None , Adr::Inherent	, Reg::A    , 2	 },
    /*4D*/  	{ Op::TST	  , Left::Ld  , Right::None , Adr::Inherent	, Reg::A    , 2	 },
    /*4E*/  	{ Op::ILL	  , Left::None, Right::None , Adr::None	    , Reg::None , 0	 },
    /*4F*/  	{ Op::CLR	  , Left::None, Right::None , Adr::Inherent	, Reg::A    , 2	 },
    /*50*/  	{ Op::NEG	  , Left::LdSt, Right::None , Adr::Inherent	, Reg::B    , 2	 },
    /*51*/  	{ Op::ILL	  , Left::None, Right::None , Adr::None	    , Reg::None , 0	 },
    /*52*/  	{ Op::ILL	  , Left::None, Right::None , Adr::None     , Reg::None , 0  },
    /*53*/  	{ Op::COM	  , Left::LdSt, Right::None , Adr::Inherent	, Reg::B    , 2	 },
    /*54*/  	{ Op::LSR	  , Left::LdSt, Right::None , Adr::Inherent	, Reg::B    , 2	 },
    /*55*/  	{ Op::ILL	  , Left::None, Right::None , Adr::None     , Reg::None , 0  },
    /*56*/  	{ Op::ROR	  , Left::LdSt, Right::None , Adr::Inherent	, Reg::B    , 2	 },
    /*57*/  	{ Op::ASR	  , Left::LdSt, Right::None , Adr::Inherent	, Reg::B    , 2	 },
    /*58*/  	{ Op::ASL	  , Left::LdSt, Right::None , Adr::Inherent	, Reg::B    , 2	 },
    /*59*/  	{ Op::ROL	  , Left::LdSt, Right::None , Adr::Inherent	, Reg::B    , 2	 },
    /*5A*/  	{ Op::DEC	  , Left::LdSt, Right::None , Adr::Inherent	, Reg::B    , 2	 },
    /*5B*/  	{ Op::ILL	  , Left::None, Right::None , Adr::None     , Reg::None , 0  },
    /*5C*/  	{ Op::INC	  , Left::LdSt, Right::None , Adr::Inherent	, Reg::B    , 2	 },
    /*5D*/  	{ Op::TST	  , Left::Ld  , Right::None , Adr::Inherent	, Reg::B    , 2	 },
    /*5E*/  	{ Op::ILL	  , Left::None, Right::None , Adr::None	    , Reg::None , 0	 },
    /*5F*/  	{ Op::CLR	  , Left::None, Right::None , Adr::Inherent	, Reg::B    , 2	 },
    /*60*/  	{ Op::NEG	  , Left::LdSt, Right::None , Adr::Indexed	, Reg::M8   , 6	 },
    /*61*/  	{ Op::ILL	  , Left::None, Right::None , Adr::None	    , Reg::None , 0	 },
    /*62*/  	{ Op::ILL	  , Left::None, Right::None , Adr::None     , Reg::None , 0  },
    /*63*/  	{ Op::COM	  , Left::LdSt, Right::None , Adr::Indexed	, Reg::M8   , 6	 },
    /*64*/  	{ Op::LSR	  , Left::LdSt, Right::None , Adr::Indexed	, Reg::M8   , 6	 },
    /*65*/  	{ Op::ILL	  , Left::None, Right::None , Adr::None     , Reg::None , 0  },
    /*66*/  	{ Op::ROR	  , Left::LdSt, Right::None , Adr::Indexed	, Reg::M8   , 6	 },
    /*67*/  	{ Op::ASR	  , Left::LdSt, Right::None , Adr::Indexed	, Reg::M8   , 6	 },
    /*68*/  	{ Op::ASL	  , Left::LdSt, Right::None , Adr::Indexed	, Reg::M8   , 6	 },
    /*69*/  	{ Op::ROL	  , Left::LdSt, Right::None , Adr::Indexed	, Reg::M8   , 6	 },
    /*6A*/  	{ Op::DEC	  , Left::LdSt, Right::None , Adr::Indexed	, Reg::M8   , 6	 },
    /*6B*/  	{ Op::ILL	  , Left::None, Right::None , Adr::None     , Reg::None , 0  },
    /*6C*/  	{ Op::INC	  , Left::LdSt, Right::None , Adr::Indexed	, Reg::M8   , 6	 },
    /*6D*/  	{ Op::TST	  , Left::Ld  , Right::None , Adr::Indexed	, Reg::M8   , 6	 },
    /*6E*/  	{ Op::JMP	  , Left::None, Right::None , Adr::Indexed	, Reg::None , 3	 },
    /*6F*/  	{ Op::CLR	  , Left::None, Right::None , Adr::Indexed	, Reg::M8   , 6	 },
    /*70*/  	{ Op::NEG	  , Left::LdSt, Right::None , Adr::Extended	, Reg::M8   , 7	 },
    /*71*/  	{ Op::ILL	  , Left::None, Right::None , Adr::None	    , Reg::None , 0	 },
    /*72*/  	{ Op::ILL	  , Left::None, Right::None , Adr::None     , Reg::None , 0  },
    /*73*/  	{ Op::COM	  , Left::LdSt, Right::None , Adr::Extended	, Reg::M8   , 7	 },
    /*74*/  	{ Op::LSR	  , Left::LdSt, Right::None , Adr::Extended	, Reg::M8   , 7	 },
    /*75*/  	{ Op::ILL	  , Left::None, Right::None , Adr::None     , Reg::None , 0  },
    /*76*/  	{ Op::ROR	  , Left::LdSt, Right::None , Adr::Extended	, Reg::M8   , 7	 },
    /*77*/  	{ Op::ASR	  , Left::LdSt, Right::None , Adr::Extended	, Reg::M8   , 7	 },
    /*78*/  	{ Op::ASL	  , Left::LdSt, Right::None , Adr::Extended	, Reg::M8   , 7	 },
    /*79*/  	{ Op::ROL	  , Left::LdSt, Right::None , Adr::Extended	, Reg::M8   , 7	 },
    /*7A*/  	{ Op::DEC	  , Left::LdSt, Right::None , Adr::Extended	, Reg::M8   , 7	 },
    /*7B*/  	{ Op::ILL	  , Left::None, Right::None , Adr::None     , Reg::None , 0  },
    /*7C*/  	{ Op::INC	  , Left::LdSt, Right::None , Adr::Extended	, Reg::M8   , 7	 },
    /*7D*/  	{ Op::TST	  , Left::Ld  , Right::None , Adr::Extended	, Reg::M8   , 7	 },
    /*7E*/  	{ Op::JMP	  , Left::None, Right::None , Adr::Extended	, Reg::None , 4	 },
    /*7F*/  	{ Op::CLR	  , Left::None, Right::None , Adr::Extended	, Reg::M8   , 7	 },
    /*80*/  	{ Op::SUB8	  , Left::LdSt, Right::None , Adr::Immed8	, Reg::A    , 2	 },
    /*81*/  	{ Op::CMP8	  , Left::Ld  , Right::None , Adr::Immed8	, Reg::A    , 2	 },
    /*82*/  	{ Op::SBC	  , Left::LdSt, Right::None , Adr::Immed8	, Reg::A    , 2	 },
    /*83*/  	{ Op::SUB16	  , Left::LdSt, Right::None , Adr::Immed16	, Reg::D    , 4	 },
    /*84*/  	{ Op::AND	  , Left::LdSt, Right::None , Adr::Immed8	, Reg::A    , 2	 },
    /*85*/  	{ Op::BIT	  , Left::Ld  , Right::None , Adr::Immed8	, Reg::A    , 2	 },
    /*86*/  	{ Op::LD8	  , Left::St  , Right::None , Adr::Immed8	, Reg::A    , 2	 },
    /*87*/  	{ Op::ILL	  , Left::None, Right::None , Adr::None     , Reg::None , 0  },
    /*88*/  	{ Op::EOR	  , Left::LdSt, Right::None , Adr::Immed8	, Reg::A    , 2	 },
    /*89*/  	{ Op::ADC	  , Left::LdSt, Right::None , Adr::Immed8	, Reg::A    , 2	 },
    /*8A*/  	{ Op::OR	  , Left::LdSt, Right::None , Adr::Immed8	, Reg::A    , 2	 },
    /*8B*/  	{ Op::ADD8	  , Left::LdSt, Right::None , Adr::Immed8	, Reg::A    , 2	 },
    /*8C*/  	{ Op::CMP16	  , Left::Ld  , Right::None , Adr::Immed16	, Reg::X    , 4	 },
    /*8D*/  	{ Op::BSR	  , Left::None, Right::None , Adr::Rel      , Reg::None , 7	 },
    /*8E*/  	{ Op::LD16	  , Left::St  , Right::None , Adr::Immed16	, Reg::X    , 3	 },
    /*8F*/  	{ Op::ILL	  , Left::None, Right::None , Adr::None     , Reg::None , 0  },
    /*90*/  	{ Op::SUB8	  , Left::LdSt, Right::Ld8  , Adr::Direct	, Reg::A    , 4	 },
    /*91*/  	{ Op::CMP8	  , Left::Ld  , Right::Ld8  , Adr::Direct	, Reg::A    , 4	 },
    /*92*/  	{ Op::SBC	  , Left::LdSt, Right::Ld8  , Adr::Direct	, Reg::A    , 4	 },
    /*93*/  	{ Op::SUB16	  , Left::LdSt, Right::Ld8  , Adr::Direct	, Reg::D    , 6	 },
    /*94*/  	{ Op::AND	  , Left::LdSt, Right::Ld8  , Adr::Direct	, Reg::A    , 4	 },
    /*95*/  	{ Op::BIT	  , Left::Ld  , Right::Ld8  , Adr::Direct	, Reg::A    , 4	 },
    /*96*/  	{ Op::LD8	  , Left::St  , Right::Ld8  , Adr::Direct	, Reg::A    , 4	 },
    /*97*/  	{ Op::ST8	  , Left::Ld  , Right::St8  , Adr::Direct	, Reg::A    , 4	 },
    /*98*/  	{ Op::EOR	  , Left::LdSt, Right::Ld8  , Adr::Direct	, Reg::A    , 4	 },
    /*99*/  	{ Op::ADC	  , Left::LdSt, Right::Ld8  , Adr::Direct	, Reg::A    , 4	 },
    /*9A*/  	{ Op::OR	  , Left::LdSt, Right::Ld8  , Adr::Direct	, Reg::A    , 4	 },
    /*9B*/  	{ Op::ADD8	  , Left::LdSt, Right::Ld8  , Adr::Direct	, Reg::A    , 4	 },
    /*9C*/  	{ Op::CMP16	  , Left::Ld  , Right::Ld8  , Adr::Direct	, Reg::X    , 6	 },
    /*9D*/  	{ Op::JSR	  , Left::None, Right::None , Adr::Direct	, Reg::None , 7	 },
    /*9E*/  	{ Op::LD16	  , Left::St  , Right::Ld16 , Adr::Direct	, Reg::X    , 5	 },
    /*9F*/  	{ Op::ST16	  , Left::Ld  , Right::St16 , Adr::Direct	, Reg::X    , 5	 },
    /*A0*/  	{ Op::SUB8	  , Left::LdSt, Right::Ld8  , Adr::Indexed	, Reg::A    , 4	 },
    /*A1*/  	{ Op::CMP8	  , Left::Ld  , Right::Ld8  , Adr::Indexed	, Reg::A    , 4	 },
    /*A2*/  	{ Op::SBC	  , Left::LdSt, Right::Ld8  , Adr::Indexed	, Reg::A    , 4	 },
    /*A3*/  	{ Op::SUB16	  , Left::LdSt, Right::Ld16 , Adr::Indexed	, Reg::D    , 6	 },
    /*A4*/  	{ Op::AND	  , Left::LdSt, Right::Ld8  , Adr::Indexed	, Reg::A    , 4	 },
    /*A5*/  	{ Op::BIT	  , Left::Ld  , Right::Ld8  , Adr::Indexed	, Reg::A    , 4	 },
    /*A6*/  	{ Op::LD8	  , Left::St  , Right::Ld8  , Adr::Indexed	, Reg::A    , 4	 },
    /*A7*/  	{ Op::ST8	  , Left::Ld  , Right::St8  , Adr::Indexed	, Reg::A    , 4	 },
    /*A8*/  	{ Op::EOR	  , Left::LdSt, Right::Ld8  , Adr::Indexed	, Reg::A    , 4	 },
    /*A9*/  	{ Op::ADC	  , Left::LdSt, Right::Ld8  , Adr::Indexed	, Reg::A    , 4	 },
    /*AA*/  	{ Op::OR	  , Left::LdSt, Right::Ld8  , Adr::Indexed	, Reg::A    , 4	 },
    /*AB*/  	{ Op::ADD8	  , Left::LdSt, Right::Ld8  , Adr::Indexed	, Reg::A    , 4	 },
    /*AC*/  	{ Op::CMP16	  , Left::Ld  , Right::Ld16 , Adr::Indexed	, Reg::X    , 6	 },
    /*AD*/  	{ Op::JSR	  , Left::None, Right::None , Adr::Indexed	, Reg::None , 7	 },
    /*AE*/  	{ Op::LD16	  , Left::St  , Right::Ld16 , Adr::Indexed	, Reg::X    , 5	 },
    /*AF*/  	{ Op::ST16	  , Left::Ld  , Right::St16 , Adr::Indexed	, Reg::X    , 5	 },
    /*B0*/  	{ Op::SUB8	  , Left::LdSt, Right::Ld8  , Adr::Extended	, Reg::A    , 5	 },
    /*B1*/  	{ Op::CMP8	  , Left::Ld  , Right::Ld8  , Adr::Extended	, Reg::A    , 5	 },
    /*B2*/  	{ Op::SBC	  , Left::LdSt, Right::Ld8  , Adr::Extended	, Reg::A    , 5	 },
    /*B3*/  	{ Op::SUB16	  , Left::LdSt, Right::Ld16 , Adr::Extended	, Reg::D    , 7	 },
    /*B4*/  	{ Op::AND	  , Left::LdSt, Right::Ld8  , Adr::Extended	, Reg::A    , 5	 },
    /*B5*/  	{ Op::BIT	  , Left::Ld  , Right::Ld8  , Adr::Extended	, Reg::A    , 5	 },
    /*B6*/  	{ Op::LD8	  , Left::St  , Right::Ld8  , Adr::Extended	, Reg::A    , 5	 },
    /*B7*/  	{ Op::ST8	  , Left::Ld  , Right::St8  , Adr::Extended	, Reg::A    , 5	 },
    /*B8*/  	{ Op::EOR	  , Left::LdSt, Right::Ld8  , Adr::Extended	, Reg::A    , 5	 },
    /*B9*/  	{ Op::ADC	  , Left::LdSt, Right::Ld8  , Adr::Extended	, Reg::A    , 5	 },
    /*BA*/  	{ Op::OR	  , Left::LdSt, Right::Ld8  , Adr::Extended	, Reg::A    , 5	 },
    /*BB*/  	{ Op::ADD8	  , Left::LdSt, Right::Ld8  , Adr::Extended	, Reg::A    , 5	 },
    /*BC*/  	{ Op::CMP16	  , Left::Ld  , Right::Ld16 , Adr::Extended	, Reg::X    , 7	 },
    /*BD*/  	{ Op::JSR	  , Left::None, Right::None , Adr::Extended	, Reg::None , 8	 },
    /*BE*/  	{ Op::LD16	  , Left::St  , Right::Ld16 , Adr::Extended	, Reg::X    , 6	 },
    /*BF*/  	{ Op::ST16	  , Left::Ld  , Right::St16 , Adr::Extended	, Reg::X    , 6	 },
    /*C0*/  	{ Op::SUB8	  , Left::LdSt, Right::None , Adr::Immed8	, Reg::B    , 2	 },
    /*C1*/  	{ Op::CMP8	  , Left::Ld  , Right::None , Adr::Immed8	, Reg::B    , 2	 },
    /*C2*/  	{ Op::SBC	  , Left::LdSt, Right::None , Adr::Immed8	, Reg::B    , 2	 },
    /*C3*/  	{ Op::ADD16	  , Left::LdSt, Right::None , Adr::Immed16	, Reg::D    , 4	 },
    /*C4*/  	{ Op::AND	  , Left::LdSt, Right::None , Adr::Immed8	, Reg::B    , 2	 },
    /*C5*/  	{ Op::BIT	  , Left::Ld  , Right::None , Adr::Immed8	, Reg::B    , 2	 },
    /*C6*/  	{ Op::LD8	  , Left::St  , Right::None , Adr::Immed8	, Reg::B    , 2	 },
    /*C7*/  	{ Op::ILL	  , Left::None, Right::None , Adr::None     , Reg::None , 0  },
    /*C8*/  	{ Op::EOR	  , Left::LdSt, Right::None , Adr::Immed8	, Reg::B    , 2	 },
    /*C9*/  	{ Op::ADC	  , Left::LdSt, Right::None , Adr::Immed8	, Reg::B    , 2	 },
    /*CA*/  	{ Op::OR	  , Left::LdSt, Right::None , Adr::Immed8	, Reg::B    , 2	 },
    /*CB*/  	{ Op::ADD8	  , Left::LdSt, Right::None , Adr::Immed8	, Reg::B    , 2	 },
    /*CC*/  	{ Op::LD16	  , Left::St  , Right::None , Adr::Immed16	, Reg::D    , 3	 },
    /*CD*/  	{ Op::ILL	  , Left::None, Right::None , Adr::None     , Reg::None , 0  },
    /*CE*/  	{ Op::LD16	  , Left::St  , Right::None , Adr::Immed16	, Reg::U    , 3	 },
    /*CF*/  	{ Op::ILL	  , Left::None, Right::None , Adr::None     , Reg::None , 0  },
    /*D0*/  	{ Op::SUB8	  , Left::LdSt, Right::Ld8  , Adr::Direct	, Reg::B    , 4	 },
    /*D1*/  	{ Op::CMP8	  , Left::Ld  , Right::Ld8  , Adr::Direct	, Reg::B    , 4	 },
    /*D2*/  	{ Op::SBC	  , Left::LdSt, Right::Ld8  , Adr::Direct	, Reg::B    , 4	 },
    /*D3*/  	{ Op::ADD16	  , Left::LdSt, Right::Ld16 , Adr::Direct	, Reg::D    , 6	 },
    /*D4*/  	{ Op::AND	  , Left::LdSt, Right::Ld8  , Adr::Direct	, Reg::B    , 4	 },
    /*D5*/  	{ Op::BIT	  , Left::Ld  , Right::Ld8  , Adr::Direct	, Reg::B    , 4	 },
    /*D6*/  	{ Op::LD8	  , Left::St  , Right::Ld8  , Adr::Direct	, Reg::B    , 4	 },
    /*D7*/  	{ Op::ST8	  , Left::Ld  , Right::St8  , Adr::Direct	, Reg::B    , 4	 },
    /*D8*/  	{ Op::EOR	  , Left::LdSt, Right::Ld8  , Adr::Direct	, Reg::B    , 4	 },
    /*D9*/  	{ Op::ADC	  , Left::LdSt, Right::Ld8  , Adr::Direct	, Reg::B    , 4	 },
    /*DA*/  	{ Op::OR	  , Left::LdSt, Right::Ld8  , Adr::Direct	, Reg::B    , 4	 },
    /*DB*/  	{ Op::ADD8	  , Left::LdSt, Right::Ld8  , Adr::Direct	, Reg::B    , 4	 },
    /*DC*/  	{ Op::LD16	  , Left::St  , Right::Ld16 , Adr::Direct	, Reg::D    , 5	 },
    /*DD*/  	{ Op::ST16	  , Left::Ld  , Right::St16 , Adr::Direct	, Reg::D    , 5	 },
    /*DE*/  	{ Op::LD16	  , Left::St  , Right::Ld16 , Adr::Direct	, Reg::U    , 5	 },
    /*DF*/  	{ Op::ST16	  , Left::Ld  , Right::St16 , Adr::Direct	, Reg::U    , 5	 },
    /*E0*/  	{ Op::SUB8	  , Left::LdSt, Right::Ld8  , Adr::Indexed	, Reg::B    , 4	 },
    /*E1*/  	{ Op::CMP8	  , Left::Ld  , Right::Ld8  , Adr::Indexed	, Reg::B    , 4	 },
    /*E2*/  	{ Op::SBC	  , Left::LdSt, Right::Ld8  , Adr::Indexed	, Reg::B    , 4	 },
    /*E3*/  	{ Op::ADD16	  , Left::LdSt, Right::Ld16 , Adr::Indexed	, Reg::D    , 6	 },
    /*E4*/  	{ Op::AND	  , Left::LdSt, Right::Ld8  , Adr::Indexed	, Reg::B    , 4	 },
    /*E5*/  	{ Op::BIT	  , Left::Ld  , Right::Ld8  , Adr::Indexed	, Reg::B    , 4	 },
    /*E6*/  	{ Op::LD8	  , Left::St  , Right::Ld8  , Adr::Indexed	, Reg::B    , 4	 },
    /*E7*/  	{ Op::ST8	  , Left::Ld  , Right::St8  , Adr::Indexed	, Reg::B    , 4	 },
    /*E8*/  	{ Op::EOR	  , Left::LdSt, Right::Ld8  , Adr::Indexed	, Reg::B    , 4	 },
    /*E9*/  	{ Op::ADC	  , Left::LdSt, Right::Ld8  , Adr::Indexed	, Reg::B    , 4	 },
    /*EA*/  	{ Op::OR	  , Left::LdSt, Right::Ld8  , Adr::Indexed	, Reg::B    , 4	 },
    /*EB*/  	{ Op::ADD8	  , Left::LdSt, Right::Ld8  , Adr::Indexed	, Reg::B    , 4	 },
    /*EC*/  	{ Op::LD16	  , Left::St  , Right::Ld16 , Adr::Indexed	, Reg::D    , 5	 },
    /*ED*/  	{ Op::ST16	  , Left::Ld  , Right::St16 , Adr::Indexed	, Reg::D    , 5	 },
    /*EE*/  	{ Op::LD16	  , Left::St  , Right::Ld16 , Adr::Indexed	, Reg::U    , 5	 },
    /*EF*/  	{ Op::ST16	  , Left::Ld  , Right::St16 , Adr::Indexed	, Reg::U    , 5	 },
    /*F0*/  	{ Op::SUB8	  , Left::LdSt, Right::Ld8  , Adr::Extended , Reg::B    , 5	 },
    /*F1*/  	{ Op::CMP8	  , Left::Ld  , Right::Ld8  , Adr::Extended , Reg::B    , 5	 },
    /*F2*/  	{ Op::SBC	  , Left::LdSt, Right::Ld8  , Adr::Extended , Reg::B    , 5	 },
    /*F3*/  	{ Op::ADD16	  , Left::LdSt, Right::Ld16 , Adr::Extended , Reg::D    , 7	 },
    /*F4*/  	{ Op::AND	  , Left::LdSt, Right::Ld8  , Adr::Extended , Reg::B    , 5	 },
    /*F5*/  	{ Op::BIT	  , Left::Ld  , Right::Ld8  , Adr::Extended , Reg::B    , 5	 },
    /*F6*/  	{ Op::LD8	  , Left::St  , Right::Ld8  , Adr::Extended , Reg::B    , 5	 },
    /*F7*/  	{ Op::ST8	  , Left::Ld  , Right::St8  , Adr::Extended , Reg::B    , 5	 },
    /*F8*/  	{ Op::EOR	  , Left::LdSt, Right::Ld8  , Adr::Extended , Reg::B    , 5	 },
    /*F9*/  	{ Op::ADC	  , Left::LdSt, Right::Ld8  , Adr::Extended , Reg::B    , 5	 },
    /*FA*/  	{ Op::OR	  , Left::LdSt, Right::Ld8  , Adr::Extended , Reg::B    , 5	 },
    /*FB*/  	{ Op::ADD8	  , Left::LdSt, Right::Ld8  , Adr::Extended , Reg::B    , 5	 },
    /*FC*/  	{ Op::LD16	  , Left::St  , Right::Ld16 , Adr::Extended , Reg::D    , 6	 },
    /*FD*/  	{ Op::ST16	  , Left::Ld  , Right::St16 , Adr::Extended , Reg::D    , 6	 },
    /*FE*/  	{ Op::LD16	  , Left::St  , Right::Ld16 , Adr::Extended , Reg::U    , 6	 },
    /*FF*/  	{ Op::ST16	  , Left::Ld  , Right::St16 , Adr::Extended , Reg::U    , 6	 },
};

static_assert (sizeof(opcodeTable) == 256 * sizeof(Opcode), "Opcode table is wrong size");

static inline uint16_t concat(uint8_t a, uint8_t b)
{
    return (uint16_t(a) << 8) | uint16_t(b);
}

uint16_t Emulator::load(std::istream& stream)
{
    SRecordInfo sRecInfo(_ram);
    std::string line;
    unsigned lineNum = 0;
    
    while (std::getline(stream, line)) {
        lineNum++;
        sRecInfo.ParseLine(lineNum, line.c_str());
    }
    sRecInfo.Flush();
    return sRecInfo.startAddr();
}

bool Emulator::execute(uint16_t addr, bool startInMonitor)
{
    _core.setStartInMonitor(startInMonitor);
    _pc = addr;
    Op prevOp = Op::NOP;
    uint16_t ea = 0;
    
    while(true) {
        uint8_t opIndex = next8();
        
        const Opcode* opcode = &(opcodeTable[opIndex]);
                
        // Handle address modes
        // If this is an addressing mode that produces a 16 bit effective address
        // it will be placed in ea. If it's immediate or branch relative then the
        // 8 or 16 bit value is placed in _right
        switch(opcode->adr) {
            case Adr::None:
            case Adr::Inherent:
                break;
            case Adr::Direct:
                ea = concat(_dp, next8());
                break;
            case Adr::Extended:
                ea = next16();
                break;
            case Adr::Immed8:
                _right = next8();
                break;
            case Adr::Immed16:
                _right = next16();
                break;
                
            // All the relative addressing modes need to be sign extended to 32 bits
            case Adr::RelL:
                _right = int16_t(next16());
                break;
            case Adr::Rel:
                _right = int8_t(next8());
                break;
          case Adr::RelP:
                if (prevOp == Op::Page2) {
                    _right = int16_t(next16());
                } else {
                    _right = int8_t(next8());
                }
                break;
            case Adr::Indexed: {
                uint8_t postbyte = next8();
                uint16_t* reg = nullptr;
                
                // Load value of RR reg in ea
                switch (RR(postbyte & 0b01100000)) {
                    case RR::X: reg = &_x; break;
                    case RR::Y: reg = &_y; break;
                    case RR::U: reg = &_u; break;
                    case RR::S: reg = &_s; break;
                }
                
                if ((postbyte & 0x80) == 0) {
                    // Constant offset direct (5 bit signed)
                    int8_t offset = postbyte & 0x1f;
                    if (offset & 0x10) {
                        offset |= 0xe0;
                    }
                    
                    ea = *reg + offset;
                } else {
                    switch(IdxMode(postbyte & IdxModeMask)) {
                        case IdxMode::ConstRegNoOff   : break;
                        case IdxMode::ConstReg8Off    : ea = *reg + int8_t(load8(_pc)); _pc += 1; break;
                        case IdxMode::ConstReg16Off   : ea = *reg + int16_t(load16(_pc)); _pc += 2; break;
                        case IdxMode::AccAOffReg      : ea = *reg + int8_t(_a); break;
                        case IdxMode::AccBOffReg      : ea = *reg + int8_t(_b); break;
                        case IdxMode::AccDOffReg      : ea = *reg + int16_t(_d); break;
                        case IdxMode::Inc1Reg         : ea = *reg; (*reg) += 1; break;
                        case IdxMode::Inc2Reg         : ea = *reg; (*reg) += 2; break;
                        case IdxMode::Dec1Reg         : (*reg) += 1; ea = *reg; break;
                        case IdxMode::Dec2Reg         : (*reg) += 2; ea = *reg; break;
                        case IdxMode::ConstPC8Off     : ea = _pc + int8_t(load8(_pc)); _pc += 1; break;
                        case IdxMode::ConstPC16Off    : ea = _pc + int16_t(load16(_pc)); _pc += 2; break;
                        case IdxMode::Extended        : ea = next16();
                    }
                    
                    if (postbyte & IndexedIndMask) {
                        // indirect from ea
                        ea = load16(ea);
                    }
                }
                    
                break;
            }
        }
        
        // Get left operand
        if (opcode->left == Left::Ld || opcode->left == Left::LdSt) {
            if (opcode->reg == Reg::M8) {
                _left = load8(ea);
            } else if (opcode->reg == Reg::M16) {
                _left = load16(ea);
            } else {
                _left = getReg(opcode->reg);
            }
        }
        
        // Get right operand
        if (opcode->right == Right::Ld8) {
            _right = load8(ea);
        } else if (opcode->right == Right::Ld16) {
            _right = load16(ea);
        }
                
        // Perform operation
        switch(opcode->op) {
            case Op::ILL:
                return false;
            case Op::Page2:
            case Op::Page3:
                break;

            case Op::BHS:
            case Op::BCC: if (!_cc.C) _pc += _right; break;
            case Op::BLO:
            case Op::BCS: if (_cc.C) _pc += _right; break;
            case Op::BEQ: if (_cc.Z) _pc += _right; break;
            case Op::BGE: if (!NxorV()) _pc += _right; break;
            case Op::BGT: if (!(NxorV() || _cc.Z)) _pc += _right; break;
            case Op::BHI: if (!_cc.C && !_cc.Z) _pc += _right; break;
            case Op::BLE: if (NxorV() || _cc.Z) _pc += _right; break;
            case Op::BLS: if (_cc.C || _cc.Z) _pc += _right; break;
            case Op::BLT: if (NxorV()) _pc += _right; break;
            case Op::BMI: if (_cc.N) _pc += _right; break;
            case Op::BNE: if (!_cc.Z) _pc += _right; break;
            case Op::BPL: if (!_cc.N) _pc += _right; break;
            case Op::BRA: _pc += _right; break;
            case Op::BRN: break;
            case Op::BVC: if (!_cc.V) _pc += _right; break;
            case Op::BVS: if (_cc.V) _pc += _right; break;
            case Op::BSR:
                push16(_s, _pc);
                _pc += _right;
                break;

            case Op::ABX:
                _x = _x + uint16_t(_b);
                break;
            case Op::ADC:
                _result = _left + _right + (_cc.C ? 1 : 0);
                HNZVC8();
                break;
            case Op::ADD8:
                _result = _left + _right;
                HNZVC8();
                break;
            case Op::ADD16:
                _result = _left + _right;
                xNZVC16();
                break;
            case Op::AND:
                _result = _left & _right;
                xNZ0x8();
                break;
            case Op::ANDCC:
                _ccByte &= _right;
                break;
            case Op::ASL:
                _result = _right << 1;
                xNZxC8();
                _cc.V = (((_left & 0x40) >> 6) ^ ((_left & 0x80) >> 7)) != 0;
                break;
            case Op::ASR:
                _result = int16_t(_right) >> 1;
                xNZxC8();
                break;
            case Op::BIT:
                _result = _left ^ _right;
                xNZ0x8();
                break;
            case Op::CLR:
                _result = 0;
                _cc.N = false;
                _cc.Z = true;
                _cc.V = false;
                _cc.C = false;
                break;
            case Op::CMP8:
            case Op::SUB8:
                _result = _left - _right;
                xNZVC8();
                break;
            case Op::CMP16:
            case Op::SUB16:
                _result = _left - _right;
                xNZVC16();
                break;
            case Op::COM:
                _result = ~_right;
                xNZ018();
                break;
            case Op::CWAI:
                _ccByte ^= _right;
                _cc.E = true;
                push16(_s, _pc);
                push16(_s, _u);
                push16(_s, _y);
                push16(_s, _x);
                push8(_s, _dp);
                push8(_s, _b);
                push8(_s, _a);
                
                // Now what?
                break;
            case Op::DAA:
                // LSN
                if (_cc.H || (_a & 0x0f) > 9) {
                    _a += 6;
                }
                
                // MSN
                if (_cc.C || (_a & 0xf0) > 0x90) {
                    _a += 0x60;
                }
                xNZ0C8();
                break;
            case Op::DEC:
                _result = _left - 1;
                xNZxx8();
                _cc.V = _left == 0x80;
                break;
            case Op::EOR:
                _result = _left ^ _right;
                xNZ0x8();
                break;
            case Op::EXG: {
                uint16_t r1 = getReg(Reg(_right & 0xf));
                uint16_t r2 = getReg(Reg(_right >> 4));
                setReg(Reg(_right & 0xf), r2);
                setReg(Reg(_right >> 4), r1);
                break;
            }
            case Op::INC:
                _result = _left + 1;
                xNZxx8();
                _cc.V = _left == 0x7f;
                break;
            case Op::JMP:
                _pc = ea;
                break;
            case Op::JSR:
                if (ea >= SystemAddrStart) {
                    // This is possibly a system call
                    _core.call(this, ea);
                } else {
                    push16(_s, _pc);
                    _pc = ea;
                }
                break;
            case Op::LD8:
                _result = _right;
                xNZ0x8();
                break;
            case Op::LD16:
                _result = _right;
                xNZ0x16();
                break;
            case Op::LEA:
                _result = ea;
                if (opcode->reg == Reg::X || opcode->reg == Reg::Y) {
                    _cc.Z = _result == 0;
                }
                break;
            case Op::LSR:
                _result = _left >> 1;
                x0ZxC8();
                break;
            case Op::MUL:
                _d = _a * _b;
                _cc.Z = _d == 0;
                _cc.C = _b & 0x80;
                break;
            case Op::NEG:
                _result = -_left;
                xNZxC8();
                _cc.V = _left == 0x80;
                break;
            case Op::NOP:
                break;
            case Op::OR:
                _result = _left | _right;
                xNZ0x8();
                break;
            case Op::ORCC:
                _ccByte |= _right;
                break;
            case Op::PSH:
            case Op::PUL: {
                // bit pattern to push or pull are in _right
                uint16_t& stack = (opcode->reg == Reg::U) ? _u : _s;
                if (opcode->op == Op::PSH) {
                    if (_right & 0x80) push16(stack, _pc);
                    if (_right & 0x40) push16(stack, (opcode->reg == Reg::U) ? _s : _u);
                    if (_right & 0x20) push16(stack, _y);
                    if (_right & 0x10) push16(stack, _x);
                    if (_right & 0x08) push8(stack, _dp);
                    if (_right & 0x04) push8(stack, _b);
                    if (_right & 0x02) push8(stack, _a);
                    if (_right & 0x01) push8(stack, _ccByte);
                } else {
                    if (_right & 0x01) _ccByte = pop8(stack);
                    if (_right & 0x02) _a = pop8(stack);
                    if (_right & 0x04) _b = pop8(stack);
                    if (_right & 0x08) _dp = pop8(stack);
                    if (_right & 0x10) _x = pop16(stack);
                    if (_right & 0x20) _y = pop16(stack);
                    if (_right & 0x40) {
                        if (opcode->reg == Reg::U) {
                            _s = pop16(stack);
                        } else {
                            _u = pop16(stack);
                        }
                    }
                    if (_right & 0x80) _pc = pop16(stack);
                }
                break;
            }
            case Op::ROL:
                _result = _left << 1;
                xNZxx8();
                _cc.V = (((_left & 0x40) >> 6) ^ ((_left & 0x80) >> 7)) != 0;
                if (_cc.C) {
                    _result |= 0x01;
                }
                break;
            case Op::ROR:
                _result = _left >> 1;
                xNZxx8();
                if (_cc.C) {
                    _result |= 0x80;
                }
                break;
            case Op::RTI:
                if (_cc.E) {
                    _a = pop8(_s);
                    _b = pop8(_s);
                    _dp = pop8(_s);
                    _x = pop16(_s);
                    _y = pop16(_s);
                    _u = pop16(_s);
                }
                _pc = pop16(_s);
                break;
            case Op::RTS:
                _pc = pop16(_s);
                break;
            case Op::SBC:
                _result = _left - _right - (_cc.C ? 1 : 0);
                xNZVC8();
                break;
            case Op::SEX:
                _a = (_b & 0x80) ? 0xff : 0;
                xNZ0x8();
                break;
            case Op::ST8:
                xNZ0x8();
                break;
            case Op::ST16: // All done in pre and post processing
                xNZ0x16();
                break;
            case Op::SWI:
            case Op::SWI2:
            case Op::SWI3:
                _cc.E = true;
                push16(_s, _pc);
                push16(_s, _u);
                push16(_s, _y);
                push16(_s, _x);
                push8(_s, _dp);
                push8(_s, _b);
                push8(_s, _a);
                _cc.I = true;
                _cc.F = true;
                switch(opcode->op) {
                    case Op::SWI: _pc = load16(0xfffa); break;
                    case Op::SWI2: _pc = load16(0xfff4); break;
                    case Op::SWI3: _pc = load16(0xfff2); break;
                    default: break;
                }
                break;
            case Op::SYNC:
                // Now what?
                break;
            case Op::TFR:
                setReg(Reg(_right & 0xf), getReg(Reg(_right >> 4)));
                break;
            case Op::TST:
                _result = _left - 0;
                xNZ0x8();
                break;
            case Op::FIRQ:
            case Op::IRQ:
            case Op::NMI:
            case Op::RESTART:
                // Now what?
                break;
        }
        
        // Store _result
        if (opcode->right == Right::St8) {
            store8(ea, _left);
        } else if (opcode->right == Right::St16) {
            store16(ea, _left);
        } else if (opcode->left == Left::St || opcode->left == Left::LdSt) {
            if (opcode->right == Right::St8 || opcode->reg == Reg::M8) {
                store8(ea, _result);
            } else if (opcode->right == Right::St16 || opcode->reg == Reg::M16) {
                store16(ea, _result);
            } else {
                setReg(opcode->reg, _result);
            }
        }
        
        prevOp = opcode->op;
    }
    
    return true;
}

// TODO:
//
// - Handle SYNC and CWAI waiting for interrupt
// - Handle firing of IRQ, FIRQ, NMI and RESTART
// - Handle CC. Have a post op deal with most of it, based on value in Opcode
// - Op handling has zero or more of left, right and ea set in addr handling and reg pre op sections.
//      Do proper setting of these in all cases. Need more than op.reg to do this. Need leftReg, rightReg, etc.
// - Add a Reg.M which uses ea to load data at ea (8 and 16 bit version)
// - Add store Reg value to opcode to store value in post op

// - Did I get ROR and ROL right WRT C flag?
