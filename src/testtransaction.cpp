#include "transaction.h"
#include "crypto/falcon/falcon.h"

#include <iostream>
#include <string>

int main()
{
    std::cout << "=== BERYL TRANSACTION TEST ===\n";

    // ========================================================
    // 1. GENERATE FALCON KEY
    // ========================================================

    FalconKey key;

    if (!key.Generate())
    {
        std::cout << "KEYGEN FAIL\n";
        return 1;
    }

    std::cout << "KEYGEN OK\n";

    // ========================================================
    // 2. BUAT TRANSAKSI
    // ========================================================

    BerylTransaction tx;

    TxInput input;

    input.previousTx =
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";

    input.outputIndex = 0;

    tx.vin.push_back(input);

    TxOutput output;

    output.address =
        "berTEST0001";

    output.amount = 100;

    tx.vout.push_back(output);

    // ========================================================
    // 3. HITUNG TXID AWAL
    // ========================================================

    tx.txid = CalculateTxID(tx);

    std::cout
        << "TXID: "
        << tx.txid
        << "\n";

    // ========================================================
    // 4. SIGN
    // ========================================================

    if (!SignTransaction(tx, key))
    {
        std::cout << "SIGN FAIL\n";
        return 1;
    }

    std::cout << "SIGN OK\n";

    std::cout
        << "SIGNATURE SIZE: "
        << tx.signature.size()
        << "\n";

    std::cout
        << "PUBLIC KEY SIZE: "
        << tx.publicKey.size()
        << "\n";

    // ========================================================
    // 5. VERIFY ORIGINAL
    // ========================================================

    if (!VerifyTransactionSignature(tx))
    {
        std::cout << "VERIFY ORIGINAL FAIL\n";
        return 1;
    }

    std::cout << "VERIFY ORIGINAL OK\n";

    // ========================================================
    // 6. TAMPER AMOUNT
    // ========================================================

    BerylTransaction tamperedAmount = tx;

    tamperedAmount.vout[0].amount = 101;

    if (VerifyTransactionSignature(tamperedAmount))
    {
        std::cout << "TAMPER AMOUNT TEST FAIL\n";
        return 1;
    }

    std::cout << "TAMPER AMOUNT OK\n";

    // ========================================================
    // 7. TAMPER ADDRESS
    // ========================================================

    BerylTransaction tamperedAddress = tx;

    tamperedAddress.vout[0].address =
        "berATTACK";

    if (VerifyTransactionSignature(tamperedAddress))
    {
        std::cout << "TAMPER ADDRESS TEST FAIL\n";
        return 1;
    }

    std::cout << "TAMPER ADDRESS OK\n";

    // ========================================================
    // 8. TAMPER SIGNATURE
    // ========================================================

    BerylTransaction tamperedSignature = tx;

    if (!tamperedSignature.signature.empty())
    {
        tamperedSignature.signature[0] ^= 0x01;
    }

    if (VerifyTransactionSignature(tamperedSignature))
    {
        std::cout << "TAMPER SIGNATURE TEST FAIL\n";
        return 1;
    }

    std::cout << "TAMPER SIGNATURE OK\n";

    // ========================================================
    // 9. TAMPER PUBLIC KEY
    // ========================================================

    BerylTransaction tamperedPubKey = tx;

    if (!tamperedPubKey.publicKey.empty())
    {
        tamperedPubKey.publicKey[0] ^= 0x01;
    }

    if (VerifyTransactionSignature(tamperedPubKey))
    {
        std::cout << "TAMPER PUBLIC KEY TEST FAIL\n";
        return 1;
    }

    std::cout << "TAMPER PUBLIC KEY OK\n";

    // ========================================================
    // 10. UTXO SET TEST
    // ========================================================

    std::cout << "\n=== UTXO TEST ===\n";

    UTXOSet utxos;

    TxOutput coinbase1;
    coinbase1.address = "berALICE";
    coinbase1.amount = 1000;
    coinbase1.spent = false;

    TxOutput coinbase2;
    coinbase2.address = "berALICE";
    coinbase2.amount = 500;
    coinbase2.spent = false;

    // Tambahkan dua UTXO milik Alice.
    utxos.Add(
        "tx0001",
        0,
        coinbase1.address,
        coinbase1.amount
    );
    utxos.Add(
        "tx0002",
        1,
        coinbase2.address,
        coinbase2.amount
    );

    // Cek balance.
    uint64_t aliceBalance =
        utxos.GetBalance("berALICE");

    if (aliceBalance != 1500)
    {
        std::cout
            << "UTXO BALANCE FAIL: "
            << aliceBalance
            << "\n";
        return 1;
    }

    std::cout << "UTXO BALANCE OK\n";

    // Cek jumlah UTXO.
    auto aliceUtxos =
        utxos.GetByAddress("berALICE");

    if (aliceUtxos.size() != 2)
    {
        std::cout
            << "UTXO COUNT FAIL: "
            << aliceUtxos.size()
            << "\n";
        return 1;
    }

    std::cout << "UTXO COUNT OK\n";

    // Duplicate UTXO tidak boleh menambah saldo.
    utxos.Add(
        "tx0001",
        0,
        coinbase1.address,
        coinbase1.amount
    );

    if (utxos.GetBalance("berALICE") != 1500)
    {
        std::cout << "UTXO DUPLICATE FAIL\n";
        return 1;
    }

    std::cout << "UTXO DUPLICATE OK\n";

    // Spend UTXO pertama.
    if (!utxos.Spend("tx0001", 0))
    {
        std::cout << "UTXO SPEND FAIL\n";
        return 1;
    }

    if (utxos.GetBalance("berALICE") != 500)
    {
        std::cout
            << "UTXO BALANCE AFTER SPEND FAIL: "
            << utxos.GetBalance("berALICE")
            << "\n";
        return 1;
    }

    std::cout << "UTXO SPEND OK\n";

    // Double-spend harus ditolak.
    if (utxos.Spend("tx0001", 0))
    {
        std::cout << "DOUBLE SPEND TEST FAIL\n";
        return 1;
    }

    std::cout << "DOUBLE SPEND OK\n";

    // UTXO yang sudah spent tidak boleh ditemukan.
    TxOutput spentOutput;

    if (GetUTXO("tx0001", 0, spentOutput))
    {
        std::cout << "SPENT UTXO STILL AVAILABLE FAIL\n";
        return 1;
    }

    std::cout << "SPENT UTXO UNAVAILABLE OK\n";

    // ========================================================
    // 11. CREATE TRANSACTION TEST
    // ========================================================

    std::cout << "\n=== CREATE TRANSACTION TEST ===\n";

    UTXOSet txUtxos;

    TxOutput source1;
    source1.address = "berALICE";
    source1.amount = 1000;
    source1.spent = false;

    TxOutput source2;
    source2.address = "berALICE";
    source2.amount = 500;
    source2.spent = false;

    txUtxos.Add(
        "source001",
        0,
        source1.address,
        source1.amount
    );
    txUtxos.Add(
        "source002",
        0,
        source2.address,
        source2.amount
    );

    BerylTransaction createdTx;

    if (!CreateTransaction(
            txUtxos,
            "berALICE",
            "berBOB",
            1200,
            createdTx))
    {
        std::cout
            << "CREATE TRANSACTION FAIL\n";
        return 1;
    }

    // Harus menghasilkan 2 input karena
    // satu UTXO 1000 belum cukup untuk 1200.
    if (createdTx.vin.size() != 2)
    {
        std::cout
            << "INPUT SELECTION FAIL: "
            << createdTx.vin.size()
            << "\n";
        return 1;
    }

    std::cout << "INPUT SELECTION OK\n";

    // Harus ada output BOB sebesar 1200.
    bool foundBob = false;
    bool foundChange = false;

    for (const auto& out : createdTx.vout)
    {
        if (out.address == "berBOB" &&
            out.amount == 1200)
        {
            foundBob = true;
        }

        if (out.address == "berALICE" &&
            out.amount == 299)
        {
            foundChange = true;
        }
    }

    if (!foundBob)
    {
        std::cout
            << "RECIPIENT OUTPUT FAIL\n";
        return 1;
    }

    if (!foundChange)
    {
        std::cout
            << "CHANGE OUTPUT FAIL\n";
        return 1;
    }

    std::cout << "TRANSACTION OUTPUTS OK\n";

    // Saldo tidak cukup harus ditolak.
    BerylTransaction insufficientTx;

    if (CreateTransaction(
            txUtxos,
            "berALICE",
            "berBOB",
            2000,
            insufficientTx))
    {
        std::cout
            << "INSUFFICIENT BALANCE TEST FAIL\n";
        return 1;
    }

    std::cout
        << "INSUFFICIENT BALANCE OK\n";


    // ========================================================
    // 12. VALIDATE TRANSACTION TEST
    // ========================================================

    std::cout << "\n=== VALIDATE TRANSACTION TEST ===\n";

    UTXOSet validationUtxos;

    TxOutput aliceUtxo;
    aliceUtxo.address = "berALICE";
    aliceUtxo.amount = 1500;
    aliceUtxo.spent = false;

    validationUtxos.Add(
        "funding0001",
        0,
        aliceUtxo.address,
        aliceUtxo.amount
    );

    // Buat transaksi valid dari UTXO Alice.
    BerylTransaction validTx;

    TxInput validInput;
    validInput.previousTx = "funding0001";
    validInput.outputIndex = 0;

    validTx.vin.push_back(validInput);

    TxOutput bobOutput;
    bobOutput.address = "berBOB";
    bobOutput.amount = 1000;
    bobOutput.spent = false;

    TxOutput aliceChange;
    aliceChange.address = "berALICE";
    aliceChange.amount = 499;
    aliceChange.spent = false;

    validTx.vout.push_back(bobOutput);
    validTx.vout.push_back(aliceChange);

    // Sign dengan key Falcon yang sudah dibuat di awal test.
    if (!SignTransaction(validTx, key))
    {
        std::cout
            << "VALID TRANSACTION SIGN FAIL\n";
        return 1;
    }

    if (!ValidateTransaction(
            validationUtxos,
            validTx))
    {
        std::cout
            << "VALID TRANSACTION FAIL\n";
        return 1;
    }

    std::cout
        << "VALID TRANSACTION OK\n";

    // --------------------------------------------------------
    // UTXO tidak ada.
    // --------------------------------------------------------

    BerylTransaction missingUtxo = validTx;

    missingUtxo.vin[0].previousTx =
        "does-not-exist";

    // Signature lama menjadi tidak valid karena input berubah.
    if (ValidateTransaction(
            validationUtxos,
            missingUtxo))
    {
        std::cout
            << "MISSING UTXO TEST FAIL\n";
        return 1;
    }

    std::cout
        << "MISSING UTXO OK\n";

    // --------------------------------------------------------
    // Double input ke UTXO yang sama.
    // --------------------------------------------------------

    BerylTransaction duplicateInput = validTx;

    duplicateInput.vin.push_back(
        duplicateInput.vin[0]
    );

    // Sign ulang supaya kegagalannya bukan hanya karena
    // signature lama.
    duplicateInput.signature.clear();
    duplicateInput.publicKey.clear();

    if (!SignTransaction(
            duplicateInput,
            key))
    {
        std::cout
            << "DUPLICATE INPUT SIGN FAIL\n";
        return 1;
    }

    if (ValidateTransaction(
            validationUtxos,
            duplicateInput))
    {
        std::cout
            << "DUPLICATE INPUT TEST FAIL\n";
        return 1;
    }

    std::cout
        << "DUPLICATE INPUT OK\n";

    // --------------------------------------------------------
    // Output lebih besar daripada input.
    // --------------------------------------------------------

    BerylTransaction excessiveOutput = validTx;

    excessiveOutput.vout[0].amount = 2000;
    excessiveOutput.vout[1].amount = 0;

    excessiveOutput.signature.clear();
    excessiveOutput.publicKey.clear();

    if (!SignTransaction(
            excessiveOutput,
            key))
    {
        std::cout
            << "EXCESSIVE OUTPUT SIGN FAIL\n";
        return 1;
    }

    if (ValidateTransaction(
            validationUtxos,
            excessiveOutput))
    {
        std::cout
            << "EXCESSIVE OUTPUT TEST FAIL\n";
        return 1;
    }

    std::cout
        << "EXCESSIVE OUTPUT OK\n";

    // --------------------------------------------------------
    // Zero-value output.
    // --------------------------------------------------------

    BerylTransaction zeroOutput = validTx;

    zeroOutput.vout[0].amount = 0;

    zeroOutput.signature.clear();
    zeroOutput.publicKey.clear();

    if (!SignTransaction(
            zeroOutput,
            key))
    {
        std::cout
            << "ZERO OUTPUT SIGN FAIL\n";
        return 1;
    }

    if (ValidateTransaction(
            validationUtxos,
            zeroOutput))
    {
        std::cout
            << "ZERO OUTPUT TEST FAIL\n";
        return 1;
    }

    std::cout
        << "ZERO OUTPUT OK\n";



    // ========================================================
    // 10. TRANSACTION FEE TEST
    // ========================================================

    std::cout << "\n=== TRANSACTION FEE TEST ===\n";

    // --------------------------------------------------------
    // Input UTXO = 100 BER
    // 1 unit = 0.00000001 BER
    // Kirim = 99 BER
    // Fee = 1 unit
    // --------------------------------------------------------

    UTXOSet feeUtxos;

    TxOutput feeInput;
    feeInput.address = "berFEE_TEST";
    feeInput.amount = 100;
    feeInput.spent = false;

    const std::string feePrevTx =
        "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb";

    feeUtxos.Add(
        feePrevTx,
        0,
        feeInput.address,
        feeInput.amount
    );

    BerylTransaction feeTx;

    if (!CreateTransaction(
            feeUtxos,
            "berFEE_TEST",
            "berRECEIVER",
            99,
            feeTx))
    {
        std::cout << "FEE TRANSACTION CREATE FAIL\n";
        return 1;
    }

    // Sign transaksi fee 1 unit.
    if (!SignTransaction(feeTx, key))
    {
        std::cout << "FEE TRANSACTION SIGN FAIL\n";
        return 1;
    }

    // Input 100 - output 99 = fee 1 unit.
    uint64_t feeTotalInput = 100;
    uint64_t feeTotalOutput = 0;

    for (const auto& out : feeTx.vout)
        feeTotalOutput += out.amount;

    uint64_t feeValue =
        feeTotalInput - feeTotalOutput;

    if (feeValue != MIN_TRANSACTION_FEE)
    {
        std::cout << "FEE VALUE TEST FAIL\n";
        std::cout << "EXPECTED: "
                  << MIN_TRANSACTION_FEE
                  << "\n";
        std::cout << "ACTUAL: "
                  << feeValue
                  << "\n";
        return 1;
    }

    std::cout << "FEE = 1 UNIT OK\n";

    // Validasi harus berhasil.
    if (!ValidateTransaction(feeUtxos, feeTx))
    {
        std::cout << "MINIMUM FEE VALIDATION FAIL\n";
        return 1;
    }

    std::cout << "FEE 1 UNIT VALIDATION OK\n";

    // --------------------------------------------------------
    // Fee 0 harus FAIL.
    // Input = 100 BER
    // Output = 100 BER
    // Fee = 0
    // --------------------------------------------------------

    UTXOSet zeroFeeUtxos;

    TxOutput zeroFeeInput;
    zeroFeeInput.address = "berZERO_FEE";
    zeroFeeInput.amount = 100;
    zeroFeeInput.spent = false;

    const std::string zeroFeePrevTx =
        "cccccccccccccccccccccccccccccccccccccccc";

    zeroFeeUtxos.Add(
        zeroFeePrevTx,
        0,
        zeroFeeInput.address,
        zeroFeeInput.amount
    );

    BerylTransaction zeroFeeTx;

    TxInput zeroInput;
    zeroInput.previousTx = zeroFeePrevTx;
    zeroInput.outputIndex = 0;

    zeroFeeTx.vin.push_back(zeroInput);

    TxOutput zeroFeeOutput;
    zeroFeeOutput.address = "berRECEIVER";
    zeroFeeOutput.amount = 100;
    zeroFeeOutput.spent = false;

    zeroFeeTx.vout.push_back(zeroFeeOutput);

    if (!SignTransaction(zeroFeeTx, key))
    {
        std::cout << "ZERO FEE SIGN FAIL\n";
        return 1;
    }

    // Validasi fee 0 WAJIB gagal.
    if (ValidateTransaction(zeroFeeUtxos, zeroFeeTx))
    {
        std::cout << "ZERO FEE TEST FAIL\n";
        return 1;
    }

    std::cout << "ZERO FEE REJECTED OK\n";


    // ========================================================
    // 11. APPLY TRANSACTION TEST
    // ========================================================

    std::cout << "\n=== APPLY TRANSACTION TEST ===\n";

    UTXOSet applyUtxos;

    // Satu UTXO sumber = 100 BER.
    TxOutput applyInput;
    applyInput.address = "berALICE";
    applyInput.amount = 100;
    applyInput.spent = false;

    const std::string applyPrevTx =
        "dddddddddddddddddddddddddddddddddddddddd";

    applyUtxos.Add(
        applyPrevTx,
        0,
        applyInput.address,
        applyInput.amount
    );

    // Buat transaksi:
    // 100 input
    // 99 output
    // 1 unit fee
    BerylTransaction applyTx;

    TxInput applyTxInput;
    applyTxInput.previousTx = applyPrevTx;
    applyTxInput.outputIndex = 0;

    applyTx.vin.push_back(applyTxInput);

    TxOutput applyOutput;
    applyOutput.address = "berBOB";
    applyOutput.amount = 99;
    applyOutput.spent = false;

    applyTx.vout.push_back(applyOutput);

    // Sign transaksi.
    if (!SignTransaction(applyTx, key))
    {
        std::cout
            << "APPLY SIGN FAIL\n";
        return 1;
    }

    // Pastikan sebelum apply saldo Alice = 100.
    if (applyUtxos.GetBalance("berALICE") != 100)
    {
        std::cout
            << "APPLY INITIAL BALANCE FAIL\n";
        return 1;
    }

    // Terapkan transaksi.
    if (!ApplyTransaction(
            applyUtxos,
            applyTx))
    {
        std::cout
            << "APPLY TRANSACTION FAIL\n";
        return 1;
    }

    std::cout
        << "APPLY TRANSACTION OK\n";

    // Input UTXO harus sudah hilang.
    TxOutput spentCheck;

    if (GetUTXO(
            applyPrevTx,
            0,
            spentCheck))
    {
        // GetUTXO() global tidak merepresentasikan
        // applyUtxos, jadi pengecekan dilakukan melalui
        // saldo dan daftar UTXO lokal di bawah.
    }

    auto bobUtxos =
        applyUtxos.GetByAddress("berBOB");

    if (bobUtxos.size() != 1)
    {
        std::cout
            << "APPLY OUTPUT COUNT FAIL\n";
        return 1;
    }

    if (bobUtxos[0].amount != 99)
    {
        std::cout
            << "APPLY OUTPUT AMOUNT FAIL\n";
        return 1;
    }

    std::cout
        << "APPLY OUTPUT UTXO OK\n";

    // Alice tidak mendapatkan change karena
    // seluruh selisih 1 unit menjadi fee.
    if (applyUtxos.GetBalance("berALICE") != 0)
    {
        std::cout
            << "APPLY CHANGE BALANCE FAIL\n";
        return 1;
    }

    std::cout
        << "APPLY FEE/CHANGE OK\n";

    // Hanya UTXO Bob yang tersisa.
    auto remaining =
        applyUtxos.GetByAddress("");

    if (remaining.size() != 1)
    {
        std::cout
            << "APPLY REMAINING UTXO COUNT FAIL\n";
        return 1;
    }

    std::cout
        << "APPLY UTXO STATE OK\n";

    // Transaksi yang sama tidak boleh diterapkan
    // untuk kedua kalinya.
    if (ApplyTransaction(
            applyUtxos,
            applyTx))
    {
        std::cout
            << "DOUBLE APPLY TEST FAIL\n";
        return 1;
    }

    std::cout
        << "DOUBLE APPLY REJECTED OK\n";


    // ========================================================
    // 12. FEE CALCULATOR TEST
    // ========================================================

    std::cout << "\n=== FEE CALCULATOR TEST ===\n";

    UTXOSet calculatorUtxos;

    TxOutput calculatorInput;
    calculatorInput.address = "berFEE_CALC";
    calculatorInput.amount = 100;
    calculatorInput.spent = false;

    const std::string calculatorPrevTx =
        "eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee";

    calculatorUtxos.Add(
        calculatorPrevTx,
        0,
        calculatorInput.address,
        calculatorInput.amount
    );

    BerylTransaction calculatorTx;

    TxInput calculatorTxInput;
    calculatorTxInput.previousTx =
        calculatorPrevTx;
    calculatorTxInput.outputIndex = 0;

    calculatorTx.vin.push_back(
        calculatorTxInput
    );

    TxOutput calculatorOutput;
    calculatorOutput.address = "berBOB";
    calculatorOutput.amount = 99;
    calculatorOutput.spent = false;

    calculatorTx.vout.push_back(
        calculatorOutput
    );

    if (!SignTransaction(
            calculatorTx,
            key))
    {
        std::cout
            << "FEE CALCULATOR SIGN FAIL\n";
        return 1;
    }

    uint64_t calculatedFee =
        CalculateTransactionFee(
            calculatorUtxos,
            calculatorTx
        );

    if (calculatedFee != MIN_TRANSACTION_FEE)
    {
        std::cout
            << "FEE CALCULATOR FAIL\n";
        std::cout
            << "EXPECTED: "
            << MIN_TRANSACTION_FEE
            << "\n";
        std::cout
            << "ACTUAL: "
            << calculatedFee
            << "\n";
        return 1;
    }

    std::cout
        << "FEE CALCULATOR = 1 UNIT OK\n";

    // --------------------------------------------------------
    // Invalid transaction harus menghasilkan fee 0.
    // --------------------------------------------------------

    BerylTransaction invalidFeeTx =
        calculatorTx;

    invalidFeeTx.vout[0].amount = 100;

    invalidFeeTx.signature.clear();
    invalidFeeTx.publicKey.clear();

    if (!SignTransaction(
            invalidFeeTx,
            key))
    {
        std::cout
            << "INVALID FEE SIGN FAIL\n";
        return 1;
    }

    uint64_t invalidFee =
        CalculateTransactionFee(
            calculatorUtxos,
            invalidFeeTx
        );

    if (invalidFee != 0)
    {
        std::cout
            << "INVALID FEE TEST FAIL\n";
        return 1;
    }

    std::cout
        << "INVALID TRANSACTION FEE = 0 OK\n";

    // ========================================================
    // FINAL
    // ========================================================

    std::cout
        << "\nALL TRANSACTION SIGNATURE TESTS PASSED\n";

    return 0;
}
