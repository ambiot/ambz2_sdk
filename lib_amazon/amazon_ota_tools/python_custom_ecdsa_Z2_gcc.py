from OpenSSL import crypto
from socket import gethostname
import os
import base64

#Reading the Private key generated using openssl(should be generated using ECDSA P256 curve)
f = open("ecdsa-sha256-signer.key.pem")
pv_buf = f.read()
f.close()
priv_key = crypto.load_privatekey(crypto.FILETYPE_PEM, pv_buf)

#Reading the certificate generated using openssl(should be generated using ECDSA P256 curve)
f = open("ecdsa-sha256-signer.crt.pem")
ss_buf = f.read()
f.close()
ss_cert = crypto.load_certificate(crypto.FILETYPE_PEM, ss_buf)

#opening the firmware_is.bin and getting the number of padding bytes
fw_bin = open('../../project/realtek_amebaz2_v0_example/GCC-RELEASE/application_is/Debug/bin/firmware_is.bin', 'br').read()
fw_bin_size = os.path.getsize("../../project/realtek_amebaz2_v0_example/GCC-RELEASE/application_is/Debug/bin/firmware_is.bin")
fw_padding = 4096-(fw_bin_size%4096)

#padding 0's to make the last block of OTA equal to an integral multiple of block size
x = bytes([0] * fw_padding)

#append the 0 padding bytes 
f = open("firmware_is_pad.bin", 'wb')
f.write(fw_bin)
f.write(x)
f.close()

#Reading OTA1 binary and individually signing it using the ECDSA P256 curve
ota1_bin = open('firmware_is_pad.bin', 'br').read()
ota1_bin_size = os.path.getsize("firmware_is_pad.bin")
# sign and verify PASS
ota1_sig = crypto.sign(priv_key, ota1_bin, 'sha256')
crypto.verify(ss_cert, ota1_sig, ota1_bin, 'sha256')
ota1_sig_size = len(ota1_sig)
#print(ota1_sig_size)

#Output signature to AWS-IoT OTA-Signature
ota_sig1_base64 = base64.b64encode(ota1_sig)
print (ota_sig1_base64)
f = open("IDT-OTA-Signature", 'w', encoding='utf-8')
f.write(ota_sig1_base64.decode('utf-8'))
f.close()

#Add image size in the begin and append signatures of ota1 and it signature sizes
#f = open("firmware_is_sig.bin", 'wb')
#int_bytes = ota1_bin_size.to_bytes(4, 'little')
#f.write(int_bytes)
#f.write(ota1_bin)
#f.write(ota1_sig)
#f.write(bytes([ota1_sig_size]))
#f.close()

#os.remove('../../project/realtek_amebaz2_v0_example/GCC-RELEASE/application_is/Debug/bin/firmware_is_pad.bin')
