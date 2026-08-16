CXX := c++
CC  := cc

CXXFLAGS := -std=c++17 -O2 -Wall -Wextra \
	-I. \
	-Icrypto/falcon \
	-Icrypto/falcon/falcon512 \
	-Icrypto/falcon/PQClean \
	-Icrypto/falcon/PQClean/common \
	-Icrypto/falcon/PQClean/crypto_sign/falcon-512/clean \
	-Icrypto/BLAKE3/c

CFLAGS := -O2 -Wall -Wextra \
	-I. \
	-Iyespower \
	-Icrypto/falcon/PQClean \
	-Icrypto/falcon/PQClean/common \
	-Icrypto/falcon/PQClean/crypto_sign/falcon-512/clean \
	-Icrypto/BLAKE3/c

LDFLAGS :=
LDLIBS := -pthread -ldl

BUILD := build

# ============================================================
# BERYL PRODUCTION SOURCES
# ============================================================

CORE_CPP := \
	blocktemplate.cpp \
	beryld.cpp \
	block.cpp \
	blockhash.cpp \
	blockvalidation.cpp \
	chain.cpp \
	coinbase.cpp \
	consensus.cpp \
	difficulty.cpp \
	genesis.cpp \
	mempool.cpp \
	merkle.cpp \
	p2p.cpp \
	pow.cpp \
	reward.cpp \
	rpc.cpp \
	storage.cpp \
	timeutil.cpp \
	transaction.cpp \
	utxo.cpp \
	utxomanager.cpp \
	utxoroot.cpp \
	validation.cpp \
	yespower_wrapper.cpp \
	bvm.cpp \
	contract.cpp \
	contract_address.cpp \
	contract_state.cpp \
	contract_state_backend.cpp \
	contract_registry.cpp \
	contract_state_manager.cpp \
	contract_context.cpp \
	contract_engine.cpp \
	crypto/blake3_wrapper.cpp \
	crypto/falcon/falcon.cpp \
	crypto/falcon/falcon512/falcon512.cpp

CORE_OBJ := $(CORE_CPP:%.cpp=$(BUILD)/%.o)

# ============================================================
# YESPOWER
# ============================================================

YESPOWER_SRC := \
	yespower/yespower-opt.c \
	yespower/sha256.c \
	yespower/yespower-platform.c

YESPOWER_OBJ := $(YESPOWER_SRC:%.c=$(BUILD)/%.o)

# ============================================================
# FALCON-512 / PQCLEAN CLEAN
# ============================================================

FALCON_SRC := \
	crypto/falcon/PQClean/crypto_sign/falcon-512/clean/codec.c \
	crypto/falcon/PQClean/crypto_sign/falcon-512/clean/common.c \
	crypto/falcon/PQClean/crypto_sign/falcon-512/clean/fft.c \
	crypto/falcon/PQClean/crypto_sign/falcon-512/clean/fpr.c \
	crypto/falcon/PQClean/crypto_sign/falcon-512/clean/keygen.c \
	crypto/falcon/PQClean/crypto_sign/falcon-512/clean/pqclean.c \
	crypto/falcon/PQClean/crypto_sign/falcon-512/clean/rng.c \
	crypto/falcon/PQClean/crypto_sign/falcon-512/clean/sign.c \
	crypto/falcon/PQClean/crypto_sign/falcon-512/clean/vrfy.c \
	crypto/falcon/PQClean/common/fips202.c \
	crypto/falcon/PQClean/common/randombytes.c

FALCON_OBJ := $(FALCON_SRC:%.c=$(BUILD)/%.o)

# ============================================================
# BLAKE3
# ============================================================

BLAKE3_SRC := \
	crypto/BLAKE3/c/blake3.c \
	crypto/BLAKE3/c/blake3_dispatch.c \
	crypto/BLAKE3/c/blake3_portable.c \
	crypto/BLAKE3/c/blake3_neon.c

BLAKE3_OBJ := $(BLAKE3_SRC:%.c=$(BUILD)/%.o)

# ============================================================
# WALLET / SEED
# ============================================================

WALLET_OBJ := \
	$(BUILD)/wallet.o \
	$(BUILD)/berylseed.o

# ============================================================
# BUILD PROGRESS
# ============================================================

TOTAL_OBJECTS := $(words $(CORE_OBJ) $(WALLET_OBJ) $(YESPOWER_OBJ) $(FALCON_OBJ) $(BLAKE3_OBJ))
PROGRESS_FILE := $(BUILD)/.progress

define SHOW_PROGRESS
	@mkdir -p $(BUILD)
	@count=$$(cat $(PROGRESS_FILE) 2>/dev/null || echo 0); \
	count=$$((count + 1)); \
	echo $$count > $(PROGRESS_FILE); \
	percent=$$((count * 100 / $(TOTAL_OBJECTS))); \
	printf "[%3d%%] %-7s %s\n" $$percent "$(1)" "$<"
endef

# ============================================================
# TARGETS
# ============================================================

.PHONY: all clean clean-yespower

all: $(BUILD)
	@rm -f $(PROGRESS_FILE)
	@$(MAKE) --no-print-directory beryld beryl-cli beryl-miner beryl-wallet

$(BUILD):
	@mkdir -p $(BUILD)

# ============================================================
# BERYL DAEMON
# ============================================================

beryld: $(CORE_OBJ) $(YESPOWER_OBJ) $(FALCON_OBJ) $(BLAKE3_OBJ)
	@echo "[100%] LINK    $@"
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

# ============================================================
# BERYL CLI
# ============================================================

beryl-cli: beryl-cli.cpp
	@echo "CXX     $<"
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS) $(LDLIBS)

# ============================================================
# BERYL MINER
# ============================================================

beryl-miner: beryl-miner.cpp \
	$(BUILD)/block.o \
	$(BUILD)/blockhash.o \
	$(BUILD)/yespower_wrapper.o \
	$(BUILD)/wallet.o \
	$(BUILD)/berylseed.o \
	$(BUILD)/crypto/falcon/falcon.o \
	$(BUILD)/crypto/falcon/falcon512/falcon512.o \
	$(BUILD)/crypto/blake3_wrapper.o \
	$(YESPOWER_OBJ) \
	$(FALCON_OBJ) \
	$(BLAKE3_OBJ)
	@echo "CXX     $<"
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

# ============================================================
# BERYL WALLET
# ============================================================

beryl-wallet: beryl-wallet.cpp \
	$(BUILD)/wallet.o \
	$(BUILD)/berylseed.o \
	$(BUILD)/transaction.o \
	$(BUILD)/utxo.o \
	$(BUILD)/crypto/falcon/falcon.o \
	$(BUILD)/crypto/falcon/falcon512/falcon512.o \
	$(BUILD)/crypto/blake3_wrapper.o \
	$(YESPOWER_OBJ) \
	$(FALCON_OBJ) \
	$(BLAKE3_OBJ)
	@echo "CXX     $<"
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

# ============================================================
# C++ COMPILE
# ============================================================

$(BUILD)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(call SHOW_PROGRESS,CXX)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

# ============================================================
# C COMPILE
# ============================================================

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(call SHOW_PROGRESS,CC)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

# ============================================================
# CLEAN
# ============================================================

clean:
	rm -rf $(BUILD)
	rm -f beryld beryl-cli beryl-miner beryl-wallet

clean-yespower:
	rm -f yespower/*.o

-include $(CORE_OBJ:.o=.d)
-include $(WALLET_OBJ:.o=.d)
-include $(YESPOWER_OBJ:.o=.d)
-include $(FALCON_OBJ:.o=.d)
-include $(BLAKE3_OBJ:.o=.d)
