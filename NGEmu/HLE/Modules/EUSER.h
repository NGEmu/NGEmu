#pragma once

namespace EUSER
{
	// Some general static classes
	class Mem
	{
	public:
		inline static void FillZ(u32* aTrg, s32 aLength);
	};

	class User
	{
	public:
		static void Leave();
		static u32 Alloc(s32 aSize);
		static u32 AllocL(s32 aSize);
		static u32 AllocZ(s32 aSize);
		static u32 AllocZL(s32 aSize);
		static s32 StringLength(u16* aString);
	};

	// CBase
	class CBase
	{
	public:
	};

	// Descriptor stuff
	// Enumerators
	enum TDesType
	{
		EBufC,
		EPtrC,
		EPtr,
		EBuf,
		EBufCPtr,
	};

	// Descriptors
	class TDesC16
	{
	public:
		u16* Ptr() const;

	protected:
		inline TDesC16(s32 aType, s32 aLength);
		inline s32 Type() const;
		inline void DoSetLength(s32 aLength);

	private:
		u32 iLength : 28;
		u32 iType : 4;
	};

	class TDes16 : public TDesC16
	{
	public:
		void SetLength(s32 aLength);
		void Copy(u16* aString);

	protected:
		inline TDes16(s32 aType, s32 aLength, s32 aMaxLength);
		inline u16* WPtr() const;

		s32 iMaxLength;
	};

	class TBufBase16 : public TDes16
	{
	public:
		TBufBase16(s32 aMaxLength);
		TBufBase16(u16* aString, s32 aMaxLength);
	};

	// Character buffers and pointers
	struct SBuf16
	{
		s32 length;
		s32 maxLength;
		u16 buf[1];
	};

	// Wrapper functions
	u32 malloc(s32 aSize);
	void TBufBase16_1(u32* object, s32 aMaxLength);
	void TBufBase16_3(u32* object, u16* aString, s32 aMaxLength);
	u32 CBase_new(u32 aSize);
	u32 CBase_newL(u32 aSize);
}