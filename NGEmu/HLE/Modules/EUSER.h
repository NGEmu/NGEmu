#pragma once

namespace EUSER
{
	// Set of static functions
	class User
	{
	public:
		static s32 StringLength(u16* aString);
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
}