#pragma once
#include "Factories/Factory.h"
#include "Doom3ImporterFactory.generated.h"

UCLASS()
class UDoom3ImporterFactory : public UFactory
{
	GENERATED_BODY()
public:
	UDoom3ImporterFactory();

	virtual UObject* FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, const FString& Filename, const TCHAR* Parms, FFeedbackContext* Warn, bool& bOutOperationCanceled) override;
	
};
