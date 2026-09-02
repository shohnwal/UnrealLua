namespace UnrealLua::MetaCache
{
	const char* FUObjectPropertyWrapperAdditions = R"###(---"
--[[
Attempts to load a Lua script for this Object.
This requires the UObject to inherit the ILuaScriptable interface.

Note: this is a Lua-only function
]]
---@param self UObject
function UObject.__LoadLuaScript(self) end

--[[

Note: this is a Lua-only function
]]
---@param self UObject
---@param subObj UObject #The replicated UObject to add
---@param condition? ELifetimeCondition #Replication condition
function UObject.__AddReplicatedSubObject(self, subObj, condition) end

--[[

Note: this is a Lua-only function
]]
---@param self UObject
---@param subObj UObject #The replicated UObject to remove
function UObject.__RemoveReplicatedSubObject(self, subObj) end

--[[

Note: this is a Lua-only function
]]
---@param self UObject
---@param funcName string #func name of UFunction to call
---@param ... any #additional function call args.
function UObject.__Super(self, funcName, ...) end

--[[

Note: this is a Lua-only function
]]
---@param self UObject
---@param ... any #additional function call args.
function UObject.__SetTimer(self, funcName, ...) end

--[[

Note: this is a Lua-only function
]]
---@param self UObject
---@param ... any #additional function call args.
function UObject.__Delay(self, funcName, ...) end
	)###";
	
	const char* PrimitivesDef = R"###(---@meta

---@alias bool boolean
bool = bool
boolean = boolean
---@alias byte integer
byte = byte
---@alias int8 integer
int8 = int8
---@alias uint8 integer
uint8 = uint8
---@alias int16 integer
int16 = int16
---@alias uint16 integer
uint16 = uint16 
---@alias int32 integer
int32 = int32
---@alias uint32 integer
uint32 = uint32
---@alias int64 integer
int64 = int64
---@alias uint64 integer
uint64 = uint64
---@alias float number
float = float
---@alias double number
double = double
---@alias str string
str = str
---@alias FString string
FString = FString
---@alias FTest string
FText = FText
---@alias FName string
FName = FName)###";

	const char* FDelegatesDef = R"###(---@meta

---@class FDelegate
FDelegate = {}

---@param self FDelegate
---@param obj UObject
---@param funcname string 
function FDelegate:Bind(obj, funcname) end

---@param self FDelegate
function FDelegate:Unbind() end

---@class FMulticastDelegate
FMulticastDelegate = {}

---@param self FMulticastDelegate
---@param obj UObject
---@param funcname string 
function FMulticastDelegate:Add(obj, funcname) end

---@param self FMulticastDelegate
---@param obj UObject
---@param funcname string 
function FMulticastDelegate:AddUnique(obj, funcname) end

---@param self FMulticastDelegate
---@param obj UObject
---@param funcname string
function FMulticastDelegate:Remove(obj, funcname) end

---@param self FMulticastDelegate
---@param obj UObject
function FMulticastDelegate:RemoveAll(obj, funcname) end)###";
	
	const char* TArrayDef = R"###(---@meta

---@class TArray<T> : {[integer]:T}, {
---__index: fun(self: TArray<T>, index: int32):T?}, {
---__newindex: fun(self: TArray<T>, index: int32, value: T)}, {
---Append: fun(self: TArray<T>, toAppend: TArray<T>)}, {
---Add: fun(self: TArray<T>, item: T)}, {
---AddAt: fun(self: TArray<T>, item: T, index: int32)}, {
---Remove: fun(self: TArray<T>, toRemove: T): boolean}, {
---RemoveAt: fun(self: TArray<T>, index: int32)}, {
---Get: fun(self: TArray<T>, index: int32): T?}, {
---At: fun(self: TArray<T>, index: int32): T?}, {
---Set: fun(self: TArray<T>, index: int32, value: T)}, {
---Clear: fun(self: TArray<T>)}, {
---Empty: fun(self: TArray<T>)}, {
---Num: fun(self: TArray<T>): int32}, {
---Find: fun(self: TArray<T>, toFind: T): int32}, {
---FindLast: fun(self: TArray<T>, toFind: T): int32}, {
---Contains: fun(self: TArray<T>, toFind: T): boolean}, {
---IsEmpty: fun(self: TArray<T>): boolean}, {
---IsValidIndex: fun(self: TArray<T>, index: int32): boolean}, {
---Copy: fun(self: TArray<T>): TArray<T>}, {
---ToTable: fun(self: TArray<T>): T[]}, {
---Filter: fun(self: TArray<T>, func: (fun(item: T): boolean), inplace: boolean?): self}, {
---RemoveAll: fun(self: TArray<T>, func: fun(item: T): boolean): self}, {
---KeepAll: fun(self: TArray<T>, func: fun(item: T): boolean): self}, {
---Top: fun(self: TArray<T>): T?}, {
---Pop: fun(self: TArray<T>): T?}, {
---Last: fun(self: TArray<T>): T?}, {
---Shuffle: fun(self: TArray<T>)}, {
---ForEach: fun(self: TArray<T>, func: fun(item: T))}, {
---Any: fun(self: TArray<T>, amount: int32): T,...}

---@generic T
---@param inner `T`
---@return TArray<T>
function TArray(inner) end)###";

	const char* TSetDef = R"###(---@meta

---@class TSet<T> : {
---Add: fun(self: TSet<T>, toAdd: T): boolean }, {
---Append: fun(self: TSet<T>, toAppend: TSet<T>) }, {
---Contains: fun(self: TSet<T>, toCheck: T):boolean}, {
---Copy: fun(self: TSet<T>): TSet<T>}, {
---Difference: fun(self: TSet<T>, other: TSet<T>): TSet<T>}, {
---Empty : fun(self: TSet<T>)}, {
---Includes: fun(self: TSet<T>, other: TSet<T>): boolean}, {
---Intersect: fun(self: TSet<T>, other: TSet<T>): TSet<T>}, {
---IsEmpty: fun(self: TSet<T>): boolean}, {
---Num: fun(self: TSet<T>): int32}, {
---Remove: fun(self: TSet<T>, toRemove: T): boolean}, {
---Union: fun(self: TSet<T>, other: TSet<T>): TSet<T>}

---@generic T
---@param inner `T`
---@return TSet<T>
function TSet(inner) end)###";
	
	const char* TMapDef = R"###(---@meta

---@class TMap<K,V> : { [K]:V }, {
---Append: fun(self: TMap<K,V>, toAppend: TMap<K,V>)}, {
---Add: fun(self: TMap<K,V>, key: K, value: V)}, {
---Num: fun(self: TMap<K,V>): integer}, {
---Find: fun(self: TMap<K,V>, key: K):V?}, {
---FindAll: fun(self: TMap<K,V>, keys: K[]):V[]}, {
---FindAllToArray: fun(self: TMap<K,V>, keys: K[]):TArray<V>}, {
---Empty: fun(self: TMap<K,V>)}, {
---IsEmpty: fun(self: TMap<K,V>): boolean}, {
---Remove: fun(self: TMap<K,V>, toRemove: K): K? }, {
---RemoveAndCopy: fun(self: TMap<K,V>, toRemove: K): V? }, {
---Contains: fun(self: TMap<K,V>, toCheck: K): boolean}, {
---ToTable: fun(self: TMap<K,V>): {[K]:V} }, {
---ToValueArray: fun(self: TMap<K,V>): TArray<V>}, {
---ToKeysArray: fun(self: TMap<K,V>): TArray<K>}, {
---Filter: fun(self: TMap<K,V>, func: (fun(key: K, value: V): boolean), inplace: boolean): TMap<K,V>}, {
---Copy: fun(self: TMap<K,V>): TMap<K,V>}, {
---ForEach: fun(self: TMap<K,V>, func: fun(key: K, value: V))}, {
---AnyValues: fun(self: TMap<K,V>, amount: int32): V,...}, {
---AnyKeys: fun(self: TMap<K,V>, amount: int32): K,...}, {
---Any: fun(self: TMap<K,V>, amount: int32): {[K]:V} }

---@generic K,V
---@param keyType `K`
---@param valueType `V`
---@return TMap<K,V>?
function TMap(keyType, valueType) end)###";

	const char* StructUtilsDef = R"###(---@meta

---@class struct

---@class FInstancedStruct
---@operator call(struct):FInstancedStruct
FInstancedStruct = {}

---@generic T : struct
---@param self FInstancedStruct
---@param structtype `T`
---@return T
function FInstancedStruct:InitializeAs(structtype) end

---@generic T : struct
---@param self FInstancedStruct
---@param structtype `T`
---@return T
function FInstancedStruct:Get(structtype) end

---@generic T : struct
---@param self FInstancedStruct
---@param structtype `T`
---@return boolean
function FInstancedStruct:Is(structtype) end

---@param self FInstancedStruct
---@return bool 
function FInstancedStruct:IsValid() end

---@class TInstancedStruct<T> : FInstancedStruct, { 
---Get: fun(self: TInstancedStruct<T>):`T`}, {
---Is: fun(self: TInstancedStruct<T>, structType: struct):boolean }, {
---IsValid: fun(self: TInstancedStruct<T>): boolean}, {
---InitializeAs: fun(self: TInstancedStruct<T>, strucType: T): T }

---@class FSharedStruct
---@operator call(struct):FSharedStruct
FSharedStruct = {}

---@generic T : struct
---@param self FSharedStruct
---@param structtype `T`
---@return T
function FSharedStruct:InitializeAs(structtype) end

---@generic T : struct
---@param self FSharedStruct
---@param structtype `T`
---@return T
function FSharedStruct:Get(structtype) end

---@generic T : struct
---@param self FSharedStruct
---@param structtype `T`
---@return boolean
function FSharedStruct:Is(structtype) end

---@param self FSharedStruct
---@return bool 
function FSharedStruct:IsValid() end

---@class TSharedStruct<T> : FSharedStruct, { 
---Get: fun(self: TSharedStruct<T>):`T`}, {
---Is: fun(self: TSharedStruct<T>, structType: struct):boolean }, {
---IsValid: fun(self: TSharedStruct<T>): boolean}, {
---InitializeAs: fun(self: TSharedStruct<T>, strucType: struct): struct })###";
	
	const char* TSubclassOfDef = R"###(---@meta

---@class TSubclassOf<T> : UObject|string)###";

	const char* TWeakObjectPtrDef = R"###(---@meta
---@class TWeakObjectPtr<T> : UObject

---@class TSoftObjectPtr<T> : UObject|string)###";
	
	const char* TScriptInterfaceDef = R"###(---@meta

---@class UInterface : UObject)###";
	
	const char* GlobalFuncs = R"###(---@meta

---Returns a string-value describing the type of the passed-in value
---@param value any
---@param outputParam? boolean|integer #modify output
---@return string #Unreal type description of value
function utype(value, outputParam) end

---@generic T: UObject
---@param objectClass T
---@param outer? UObject
---@param name? string
---@param template? UObject
---@param init? table
---@return `T`
---@nodiscard
function NewObject(objectClass, outer, name, template, init) end

---@param obj UObject
---@param newName? string
---@param newOuter? UObject
function RenameObject(obj, newName, newOuter) end

---@param uobject UObject #UObject to check for validity
---@return bool
function IsValid(uobject) end

---@param self UObject #UObject to call function on
---@param ufunction string #UFunction to call in UObject
---@param ... any #UFunction arguments
function super(self, ufunction, ...) end

---@param actorClass TSubclassOf<AActor>
---@param spawnTransform FTransform?
---@param init table|function|nil?
---@param owner AActor?
---@param instigator AActor?
---@param spawnMethod ESpawnActorCollisionHandlingMethod?
---@param scaleMethod ESpawnActorScaleMethod?
---@return AActor?
function SpawnActor(actorClass, spawnTransform, init, owner, instigator, spawnMethod, scaleMethod) end

---@param actorClass TSubclassOf<AActor>
---@param spawnTransform FTransform?
---@param init table|function?
---@param owner AActor?
---@param instigator AActor?
---@param spawnMethod ESpawnActorCollisionHandlingMethod?
---@param scaleMethod ESpawnActorScaleMethod?
---@return AActor?
---@nodiscard
function SpawnActorDeferred(actorClass, spawnTransform, init, owner, instigator, spawnMethod, scaleMethod) end
	
---@param actor AActor
---@param spawnTransform FTransform?
---@param scaleMethod ESpawnActorScaleMethod?
---@return AActor?
function FinishSpawningActor(actor, spawnTransform, scaleMethod) end)###";
}