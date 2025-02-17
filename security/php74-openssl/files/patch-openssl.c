--- openssl.c.orig	2021-05-04 11:06:38 UTC
+++ openssl.c
@@ -18,6 +18,7 @@
    |          Pierre-Alain Joye <pierre@php.net>                          |
    |          Marc Delling <delling@silpion.de> (PKCS12 functions)        |
    |          Jakub Zelenka <bukka@php.net>                               |
+   |          Moritz Bechler <mbechler@eenterphace.org> (CRL support)     |
    +----------------------------------------------------------------------+
  */
 
@@ -55,6 +56,7 @@
 #include <openssl/rand.h>
 #include <openssl/ssl.h>
 #include <openssl/pkcs12.h>
+#include <openssl/ocsp.h>
 
 /* Common */
 #include <time.h>
@@ -131,6 +133,56 @@ PHP_FUNCTION(openssl_pkey_derive);
 PHP_FUNCTION(openssl_random_pseudo_bytes);
 
 /* {{{ arginfo */
+ZEND_BEGIN_ARG_INFO_EX(arginfo_openssl_crl_export_file, 0, 0, 3)
+    ZEND_ARG_INFO(0, crl)
+    ZEND_ARG_INFO(0, filename)
+    ZEND_ARG_INFO(0, capkey)
+    ZEND_ARG_INFO(0, crlv2)
+    ZEND_ARG_INFO(0, notext)
+    ZEND_ARG_INFO(0, capass)
+ZEND_END_ARG_INFO()
+
+ZEND_BEGIN_ARG_INFO_EX(arginfo_openssl_crl_export, 0, 0, 3)
+    ZEND_ARG_INFO(0, crl)
+    ZEND_ARG_INFO(1, data)
+    ZEND_ARG_INFO(0, capkey)
+    ZEND_ARG_INFO(0, crlv2)
+    ZEND_ARG_INFO(0, notext)
+    ZEND_ARG_INFO(0, capass)
+ZEND_END_ARG_INFO()
+
+ZEND_BEGIN_ARG_INFO_EX(arginfo_openssl_crl_revoke_cert, 0, 0, 3)
+    ZEND_ARG_INFO(0, crl)
+    ZEND_ARG_INFO(0, certficate)
+    ZEND_ARG_INFO(0, revokation_date)
+    ZEND_ARG_INFO(0, reason)
+    ZEND_ARG_INFO(0, compromise_date)
+    ZEND_ARG_INFO(0, hold_instruction)
+ZEND_END_ARG_INFO()
+
+ZEND_BEGIN_ARG_INFO_EX(arginfo_openssl_crl_revoke_cert_by_serial, 0, 0, 3)
+    ZEND_ARG_INFO(0, crl)
+    ZEND_ARG_INFO(0, revoke_serial)
+    ZEND_ARG_INFO(0, revokation_date)
+    ZEND_ARG_INFO(0, reason)
+    ZEND_ARG_INFO(0, compromise_date)
+    ZEND_ARG_INFO(0, hold_instruction)
+ZEND_END_ARG_INFO()
+
+ZEND_BEGIN_ARG_INFO_EX(arginfo_openssl_crl_new, 0, 0, 1)
+    ZEND_ARG_INFO(0, cacert)
+    ZEND_ARG_INFO(0, crlserial)
+    ZEND_ARG_INFO(0, lifetime)
+ZEND_END_ARG_INFO()
+
+ZEND_BEGIN_ARG_INFO_EX(arginfo_openssl_x509_check, 0, 0, 4)
+    ZEND_ARG_INFO(0, x509cert)
+    ZEND_ARG_INFO(0, purpose)
+    ZEND_ARG_INFO(0, flags)
+    ZEND_ARG_INFO(0, cainfo)
+    ZEND_ARG_INFO(0, untrustedfile)
+ZEND_END_ARG_INFO()
+
 ZEND_BEGIN_ARG_INFO_EX(arginfo_openssl_x509_export_to_file, 0, 0, 2)
 	ZEND_ARG_INFO(0, x509)
 	ZEND_ARG_INFO(0, outfilename)
@@ -502,12 +554,20 @@ static const zend_function_entry openssl_functions[] =
 	PHP_FE(openssl_x509_export,				arginfo_openssl_x509_export)
 	PHP_FE(openssl_x509_fingerprint,			arginfo_openssl_x509_fingerprint)
 	PHP_FE(openssl_x509_export_to_file,		arginfo_openssl_x509_export_to_file)
+	PHP_FE(openssl_x509_check,		arginfo_openssl_x509_check)
 
 /* PKCS12 funcs */
 	PHP_FE(openssl_pkcs12_export,			arginfo_openssl_pkcs12_export)
 	PHP_FE(openssl_pkcs12_export_to_file,	arginfo_openssl_pkcs12_export_to_file)
 	PHP_FE(openssl_pkcs12_read,				arginfo_openssl_pkcs12_read)
 
+/* for CRL creation */
+	PHP_FE(openssl_crl_new,			arginfo_openssl_crl_new)
+	PHP_FE(openssl_crl_revoke_cert_by_serial,       arginfo_openssl_crl_revoke_cert_by_serial)
+	PHP_FE(openssl_crl_revoke_cert,         arginfo_openssl_crl_revoke_cert)
+	PHP_FE(openssl_crl_export,              arginfo_openssl_crl_export)
+	PHP_FE(openssl_crl_export_file,         arginfo_openssl_crl_export_file)
+
 /* CSR funcs */
 	PHP_FE(openssl_csr_new,				arginfo_openssl_csr_new)
 	PHP_FE(openssl_csr_export,			arginfo_openssl_csr_export)
@@ -776,6 +836,7 @@ void php_openssl_store_errors()
 static int le_key;
 static int le_x509;
 static int le_csr;
+static int le_crl;
 static int ssl_stream_data_index;
 
 int php_openssl_get_x509_list_id(void) /* {{{ */
@@ -784,6 +845,16 @@ int php_openssl_get_x509_list_id(void) /* {{{ */
 }
 /* }}} */
 
+/* {{{ */
+struct php_x509_crl {
+	X509_CRL *crl;
+	X509 *cacert;
+	long lifetime;
+	int forcev2;
+};
+/* }}} */
+
+
 /* {{{ resource destructors */
 static void php_openssl_pkey_free(zend_resource *rsrc)
 {
@@ -805,6 +876,21 @@ static void php_openssl_csr_free(zend_resource *rsrc)
 	X509_REQ * csr = (X509_REQ*)rsrc->ptr;
 	X509_REQ_free(csr);
 }
+
+static void php_openssl_crl_free(zend_resource *rsrc TSRMLS_DC)
+{
+	struct php_x509_crl *res = (struct php_x509_crl*)rsrc->ptr;
+
+	if(res) {
+		if(res->crl != NULL) {
+			X509_CRL_free(res->crl);
+		}
+
+		efree(res);
+	}
+
+}
+
 /* }}} */
 
 /* {{{ openssl open_basedir check */
@@ -1452,6 +1538,7 @@ PHP_MINIT_FUNCTION(openssl)
 	le_key = zend_register_list_destructors_ex(php_openssl_pkey_free, NULL, "OpenSSL key", module_number);
 	le_x509 = zend_register_list_destructors_ex(php_openssl_x509_free, NULL, "OpenSSL X.509", module_number);
 	le_csr = zend_register_list_destructors_ex(php_openssl_csr_free, NULL, "OpenSSL X.509 CSR", module_number);
+	le_crl = zend_register_list_destructors_ex(php_openssl_crl_free, NULL, "OpenSSL X.509 CRL", module_number);
 
 #if OPENSSL_VERSION_NUMBER < 0x10100000L || defined (LIBRESSL_VERSION_NUMBER)
 	OPENSSL_config(NULL);
@@ -1554,6 +1641,36 @@ PHP_MINIT_FUNCTION(openssl)
 	REGISTER_LONG_CONSTANT("OPENSSL_ZERO_PADDING", OPENSSL_ZERO_PADDING, CONST_CS|CONST_PERSISTENT);
 	REGISTER_LONG_CONSTANT("OPENSSL_DONT_ZERO_PAD_KEY", OPENSSL_DONT_ZERO_PAD_KEY, CONST_CS|CONST_PERSISTENT);
 
+	/* OCSP revokation states */
+	REGISTER_LONG_CONSTANT("OCSP_REVOKED_STATUS_NOSTATUS", OCSP_REVOKED_STATUS_NOSTATUS, CONST_CS | CONST_PERSISTENT);
+	REGISTER_LONG_CONSTANT("OCSP_REVOKED_STATUS_UNSPECIFIED", OCSP_REVOKED_STATUS_UNSPECIFIED, CONST_CS | CONST_PERSISTENT);
+	REGISTER_LONG_CONSTANT("OCSP_REVOKED_STATUS_KEYCOMPROMISE", OCSP_REVOKED_STATUS_KEYCOMPROMISE, CONST_CS | CONST_PERSISTENT);
+	REGISTER_LONG_CONSTANT("OCSP_REVOKED_STATUS_CACOMPROMISE", OCSP_REVOKED_STATUS_CACOMPROMISE, CONST_CS | CONST_PERSISTENT);
+	REGISTER_LONG_CONSTANT("OCSP_REVOKED_STATUS_AFFILIATIONCHANGED", OCSP_REVOKED_STATUS_AFFILIATIONCHANGED, CONST_CS | CONST_PERSISTENT);
+	REGISTER_LONG_CONSTANT("OCSP_REVOKED_STATUS_SUPERSEDED", OCSP_REVOKED_STATUS_SUPERSEDED, CONST_CS | CONST_PERSISTENT);
+	REGISTER_LONG_CONSTANT("OCSP_REVOKED_STATUS_CESSATIONOFOPERATION", OCSP_REVOKED_STATUS_CESSATIONOFOPERATION, CONST_CS | CONST_PERSISTENT);
+	REGISTER_LONG_CONSTANT("OCSP_REVOKED_STATUS_CERTIFICATEHOLD", OCSP_REVOKED_STATUS_CERTIFICATEHOLD, CONST_CS | CONST_PERSISTENT);
+	REGISTER_LONG_CONSTANT("OCSP_REVOKED_STATUS_REMOVEFROMCRL", OCSP_REVOKED_STATUS_REMOVEFROMCRL, CONST_CS | CONST_PERSISTENT);
+
+	/* X509 Hold instruction NIDs */
+	REGISTER_LONG_CONSTANT("OPENSSL_HOLDINSTRUCTION_NONE", NID_hold_instruction_none, CONST_CS | CONST_PERSISTENT);
+	REGISTER_LONG_CONSTANT("OPENSSL_HOLDINSTRUCTION_CALL_ISSUER", NID_hold_instruction_call_issuer, CONST_CS | CONST_PERSISTENT);
+	REGISTER_LONG_CONSTANT("OPENSSL_HOLDINSTRUCTION_REJECT", NID_hold_instruction_reject, CONST_CS | CONST_PERSISTENT);
+
+	/* openssl_verify flags */
+	REGISTER_LONG_CONSTANT("X509_V_FLAG_CB_ISSUER_CHECK", X509_V_FLAG_CB_ISSUER_CHECK, CONST_CS | CONST_PERSISTENT);
+	REGISTER_LONG_CONSTANT("X509_V_FLAG_USE_CHECK_TIME", X509_V_FLAG_USE_CHECK_TIME, CONST_CS | CONST_PERSISTENT);
+	REGISTER_LONG_CONSTANT("X509_V_FLAG_CRL_CHECK", X509_V_FLAG_CRL_CHECK, CONST_CS | CONST_PERSISTENT);
+	REGISTER_LONG_CONSTANT("X509_V_FLAG_CRL_CHECK_ALL", X509_V_FLAG_CRL_CHECK_ALL, CONST_CS | CONST_PERSISTENT);
+	REGISTER_LONG_CONSTANT("X509_V_FLAG_IGNORE_CRITICAL", X509_V_FLAG_IGNORE_CRITICAL, CONST_CS | CONST_PERSISTENT);
+	REGISTER_LONG_CONSTANT("X509_V_FLAG_X509_STRICT", X509_V_FLAG_X509_STRICT, CONST_CS | CONST_PERSISTENT);
+	REGISTER_LONG_CONSTANT("X509_V_FLAG_ALLOW_PROXY_CERTS", X509_V_FLAG_ALLOW_PROXY_CERTS, CONST_CS | CONST_PERSISTENT);
+	REGISTER_LONG_CONSTANT("X509_V_FLAG_POLICY_CHECK", X509_V_FLAG_POLICY_CHECK, CONST_CS | CONST_PERSISTENT);
+	REGISTER_LONG_CONSTANT("X509_V_FLAG_EXPLICIT_POLICY", X509_V_FLAG_EXPLICIT_POLICY, CONST_CS | CONST_PERSISTENT);
+	REGISTER_LONG_CONSTANT("X509_V_FLAG_INHIBIT_ANY", X509_V_FLAG_INHIBIT_ANY, CONST_CS | CONST_PERSISTENT);
+	REGISTER_LONG_CONSTANT("X509_V_FLAG_INHIBIT_MAP", X509_V_FLAG_INHIBIT_MAP, CONST_CS | CONST_PERSISTENT);
+	REGISTER_LONG_CONSTANT("X509_V_FLAG_NOTIFY_POLICY", X509_V_FLAG_NOTIFY_POLICY, CONST_CS | CONST_PERSISTENT);
+
 #ifndef OPENSSL_NO_TLSEXT
 	/* SNI support included */
 	REGISTER_LONG_CONSTANT("OPENSSL_TLSEXT_SERVER_NAME", 1, CONST_CS|CONST_PERSISTENT);
@@ -2580,7 +2697,7 @@ end:
 /* }}} */
 
 /* {{{ check_cert */
-static int check_cert(X509_STORE *ctx, X509 *x, STACK_OF(X509) *untrustedchain, int purpose)
+static int check_cert(X509_STORE *ctx, X509 *x, STACK_OF(X509) *untrustedchain, int purpose, long flags)
 {
 	int ret=0;
 	X509_STORE_CTX *csc;
@@ -2596,6 +2713,9 @@ static int check_cert(X509_STORE *ctx, X509 *x, STACK_
 		php_error_docref(NULL, E_WARNING, "cert store initialization failed");
 		return 0;
 	}
+	if(flags) {
+		X509_STORE_CTX_set_flags(csc, flags);
+	}
 	if (purpose >= 0 && !X509_STORE_CTX_set_purpose(csc, purpose)) {
 		php_openssl_store_errors();
 	}
@@ -2609,6 +2729,59 @@ static int check_cert(X509_STORE *ctx, X509 *x, STACK_
 }
 /* }}} */
 
+/* {{{ proto int openssl_x509_check(mixed x509cert, int purpose, int flags, array cainfo [, string untrustedfile])
+   Checks the CERT to see if it can be used for the purpose in purpose. cainfo holds information about trusted CAs.
+   Flags should be one of the  X509_V_FLAG_* constants and controls the behaviour of the check.
+   */
+PHP_FUNCTION(openssl_x509_check)
+{
+        zval * zcert, * zcainfo = NULL;
+        X509_STORE * cainfo = NULL;
+        X509 * cert = NULL;
+        zend_resource * certresource = NULL;
+        STACK_OF(X509) * untrustedchain = NULL;
+        long purpose;
+        char * untrusted = NULL;
+        int untrusted_len;
+        long flags = 0;
+
+        if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zll|a!s", &zcert, &purpose, &flags, &zcainfo, &untrusted, &untrusted_len) == FAILURE) {
+                return;
+        }
+
+        RETVAL_LONG(-1);
+
+        if (untrusted) {
+                untrustedchain = php_openssl_load_all_certs_from_file(untrusted);
+                if (untrustedchain == NULL) {
+                        goto clean_exit;
+                }
+        }
+
+        cainfo = php_openssl_setup_verify(zcainfo TSRMLS_CC);
+        if (cainfo == NULL) {
+                goto clean_exit;
+        }
+        cert = php_openssl_x509_from_zval(zcert, 0, &certresource TSRMLS_CC);
+        if (cert == NULL) {
+                goto clean_exit;
+        }
+        RETVAL_LONG(check_cert(cainfo, cert, untrustedchain, purpose, flags));
+
+clean_exit:
+        if (certresource == NULL && cert) {
+                X509_free(cert);
+        }
+        if (cainfo) { 
+                X509_STORE_free(cainfo); 
+        }
+        if (untrustedchain) {
+                sk_X509_pop_free(untrustedchain, X509_free);
+        }
+}
+/* }}} */
+
+
 /* {{{ proto int openssl_x509_checkpurpose(mixed x509cert, int purpose, array cainfo [, string untrustedfile])
    Checks the CERT to see if it can be used for the purpose in purpose. cainfo holds information about trusted CAs */
 PHP_FUNCTION(openssl_x509_checkpurpose)
@@ -2644,7 +2817,7 @@ PHP_FUNCTION(openssl_x509_checkpurpose)
 		goto clean_exit;
 	}
 
-	ret = check_cert(cainfo, cert, untrustedchain, (int)purpose);
+	ret = check_cert(cainfo, cert, untrustedchain, (int)purpose, 0);
 	if (ret != 0 && ret != 1) {
 		RETVAL_LONG(ret);
 	} else {
@@ -7007,5 +7180,441 @@ PHP_FUNCTION(openssl_random_pseudo_bytes)
 	if (zstrong_result_returned) {
 		ZEND_TRY_ASSIGN_REF_TRUE(zstrong_result_returned);
 	}
+}
+/* }}} */
+ 
+/* CRL creation functions */
+
+/* {{{ proto resource openssl_crl_new(mixed cacert[, int crlserial[, string int lifetime]])
+   Creates a new CRL */
+PHP_FUNCTION(openssl_crl_new)
+{
+	struct php_x509_crl *res = NULL;
+	long serial = 0;
+	ASN1_INTEGER *crl_number;
+	long lifetime_days = 80;
+	zend_resource *certresource;
+	zval* zcacert;
+	
+	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z|ll", &zcacert, &serial, &lifetime_days) == FAILURE) {
+		RETURN_NULL();
+	}
+
+	res = emalloc(sizeof(struct php_x509_crl));
+	memset(res, 0, sizeof(struct php_x509_crl));
+	res->lifetime = 60*60*24*lifetime_days;
+
+	/* Initialize CRL */
+	res->crl = X509_CRL_new();
+	if(res->crl == NULL) {
+		php_error_docref(NULL TSRMLS_CC, E_WARNING,
+						"Failed to create CRL");
+		efree(res);
+		RETURN_NULL();
+	}
+
+	res->cacert = php_openssl_x509_from_zval(zcacert, 0, &certresource TSRMLS_CC);
+
+	if(!res->cacert) {
+		X509_CRL_free(res->crl);
+		efree(res);
+
+		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Failed to load CA Certificate");
+		RETURN_NULL();
+	}
+
+	if (!X509_CRL_set_issuer_name(res->crl, X509_get_subject_name(res->cacert))) {
+		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid ca certificate");
+		X509_free(res->cacert);
+		X509_CRL_free(res->crl);
+		efree(res);
+		RETURN_NULL();
+	}
+
+	/* set CRL number (serial) (forces the crl to be version 2) */
+	if(serial) {
+		crl_number = ASN1_INTEGER_new();
+		ASN1_INTEGER_set(crl_number, serial);
+
+		if(!X509_CRL_add1_ext_i2d(res->crl,NID_crl_number,crl_number,0,0)) {
+			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Failed to set CRL serial");
+			ASN1_INTEGER_free(crl_number);
+			X509_free(res->cacert);
+			X509_CRL_free(res->crl);
+			efree(res);
+			RETURN_NULL();
+		}
+		res->forcev2 = 1;
+
+		ASN1_INTEGER_free(crl_number);
+	}
+	
+	RETURN_RES(zend_register_resource(res, le_crl));
+}
+/* }}} */
+
+/* {{{ Adds an entry to the CRL */
+static int php_openssl_crl_revoke_serial(struct php_x509_crl *res, ASN1_INTEGER *serial, time_t rev_timestamp, long reason_code, time_t comp_timestamp, int hold TSRMLS_DC)
+{
+	X509_REVOKED *revoke;
+
+	ASN1_TIME *rev_date;
+	ASN1_GENERALIZEDTIME *comp_date;
+
+	ASN1_ENUMERATED *rtmp;
+	
+	ASN1_OBJECT *hold_instruction;
+	
+	if(!res) {
+		return FAILURE;
+	}
+	
+	revoke = X509_REVOKED_new();
+
+	if(!revoke) {
+		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Failed to revoke cert");
+		X509_REVOKED_free(revoke);
+		return FAILURE;
+	}
+
+	/* Add serial to the CRL */	
+	if(!X509_REVOKED_set_serialNumber(revoke, serial)) {
+		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Failed to set serial number");
+		X509_REVOKED_free(revoke);
+		return FAILURE;
+	}
+
+	/* Revokation date */
+	rev_date = ASN1_UTCTIME_new();
+	if (!ASN1_UTCTIME_set(rev_date, rev_timestamp) || !X509_REVOKED_set_revocationDate(revoke,rev_date)) {
+		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Failed to set revokation time");
+		ASN1_UTCTIME_free(rev_date);
+		X509_REVOKED_free(revoke);
+		return FAILURE;
+	}
+	ASN1_UTCTIME_free(rev_date);
+
+
+	if(reason_code && reason_code != OCSP_REVOKED_STATUS_UNSPECIFIED) {
+		rtmp = ASN1_ENUMERATED_new();
+		ASN1_ENUMERATED_set(rtmp, reason_code);
+		
+		if(!X509_REVOKED_add1_ext_i2d(revoke, NID_crl_reason, rtmp, 0, 0)) {
+			ASN1_ENUMERATED_free(rtmp);
+			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Failed to set revokation reason");
+			return FAILURE;
+		}
+		res->forcev2 = 1;
+		ASN1_ENUMERATED_free(rtmp);
+	}
+
+	/* Add compromise timestamp if reason is key or ca compromise */
+	if(comp_timestamp && (reason_code == OCSP_REVOKED_STATUS_KEYCOMPROMISE || reason_code == OCSP_REVOKED_STATUS_CACOMPROMISE)) {
+		comp_date = ASN1_GENERALIZEDTIME_new();
+		if (!ASN1_GENERALIZEDTIME_set(comp_date, comp_timestamp) ||
+				!X509_REVOKED_add1_ext_i2d(revoke, NID_invalidity_date, comp_date, 0, 0)) {
+			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Failed to set compromise time");
+			ASN1_GENERALIZEDTIME_free(comp_date);
+			return FAILURE;
+		}
+		ASN1_GENERALIZEDTIME_free(comp_date);
+	}
+	
+	/* Add hold instruction if reason is certificateHold */
+	if(reason_code == OCSP_REVOKED_STATUS_CERTIFICATEHOLD) {
+		hold_instruction = OBJ_nid2obj(hold);
+		
+		if (!X509_REVOKED_add1_ext_i2d(revoke, NID_hold_instruction_code, hold_instruction, 0, 0)) {
+			ASN1_OBJECT_free(hold_instruction);
+			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Failed to set hold instruction");
+			return FAILURE;
+		}
+		
+		
+		ASN1_OBJECT_free(hold_instruction);
+	}
+
+	X509_CRL_add0_revoked(res->crl,revoke);
+	
+	return SUCCESS;
+}
+/* }}} */
+
+/* {{{ proto bool openssl_crl_revoke_cert_by_serial(resource crl, string revoke_serial, int revokation_date[, int reason[, int compromise_date[, int hold_instruction]]])
+   Adds a certificate to the revokation list using its serial */
+PHP_FUNCTION(openssl_crl_revoke_cert_by_serial) 
+{
+
+	time_t rev_timestamp;
+	time_t comp_timestamp;
+
+	zval *crl_res;
+	struct php_x509_crl *res = NULL;
+
+	long reason_code = OCSP_REVOKED_STATUS_UNSPECIFIED;
+	zval *serial_num;
+	
+	long hold = NID_hold_instruction_reject;
+	
+	ASN1_INTEGER *serial;
+
+	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rzl|lll", 
+			&crl_res, &serial_num, &rev_timestamp, &reason_code, &comp_timestamp, &hold) == FAILURE) {
+		RETURN_FALSE;
+	}
+
+	if ((res = (struct  php_x509_crl*) zend_fetch_resource(Z_RES_P(crl_res), "OpenSSL X.509 CRL", le_crl)) == NULL) {
+		RETURN_FALSE;
+	}
+	
+	convert_to_string(serial_num);
+	
+	serial = ASN1_INTEGER_new();
+	serial = s2i_ASN1_INTEGER(NULL, Z_STRVAL_P(serial_num));
+	
+	if(php_openssl_crl_revoke_serial(res, serial, rev_timestamp, reason_code, comp_timestamp, hold TSRMLS_CC) != SUCCESS) {
+		ASN1_INTEGER_free(serial);
+		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Failed to revoke certificate");
+		RETURN_FALSE;
+	}	
+	ASN1_INTEGER_free(serial);
+
+	RETURN_TRUE;
+}
+
+/* {{{ proto bool openssl_revoke_cert(resource crl, mixed certficate, int revokation_date[, int reason[, int compromise_date[, int hold_instruction]]])
+   Adds a certificate to the revokation list using a certificate resource */
+PHP_FUNCTION(openssl_crl_revoke_cert) 
+{
+	time_t rev_timestamp;
+	time_t comp_timestamp;
+
+	zval *crl_res;
+	struct php_x509_crl *res = NULL;
+
+	long reason_code = OCSP_REVOKED_STATUS_UNSPECIFIED;
+	
+	X509 *cert;
+	zval *zcert;
+	zend_resource *certresource;
+	
+	long hold = NID_hold_instruction_reject;
+	
+	ASN1_INTEGER *serial;
+
+	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rzl|lll", 
+			&crl_res, &zcert, &rev_timestamp, &reason_code, &comp_timestamp, &hold) == FAILURE) {
+		RETURN_FALSE;
+	}
+
+	if ((res = (struct php_x509_crl *)zend_fetch_resource(Z_RES_P(crl_res), "OpenSSL X.509 CRL", le_crl)) == NULL) {
+		RETURN_FALSE;
+	}
+	
+	
+	cert = php_openssl_x509_from_zval(zcert, 0, &certresource TSRMLS_CC);	
+	serial = X509_get_serialNumber(cert);
+	
+	if(php_openssl_crl_revoke_serial(res, serial, rev_timestamp, reason_code, comp_timestamp, hold TSRMLS_CC) != SUCCESS) {
+		ASN1_INTEGER_free(serial);
+		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Failed to revoke certificate");
+		RETURN_FALSE;
+	}	
+	ASN1_INTEGER_free(serial);
+
+	RETURN_TRUE;
+}
+
+/* Generates the CRL */
+static int php_openssl_crl_generate(struct php_x509_crl *res, zval* zcapkey, int crlv2, char* capass, long capass_len, long digest_alg TSRMLS_DC) 
+{
+	zend_resource *capkeyres;
+	EVP_PKEY *capkey;
+	EVP_MD *digest;
+	ASN1_TIME *tmptm;
+	
+	
+	/* Prepare last updated and next update timestamps */
+	tmptm = ASN1_TIME_new();
+	if (!tmptm) {
+		return FAILURE;
+	}
+	X509_gmtime_adj(tmptm,0);
+	X509_CRL_set_lastUpdate(res->crl, tmptm);	
+	X509_gmtime_adj(tmptm,res->lifetime);
+	X509_CRL_set_nextUpdate(res->crl, tmptm);	
+	ASN1_TIME_free(tmptm);
+
+	/* Initialize message digest */
+	
+	
+	digest = php_openssl_get_evp_md_from_algo(digest_alg);
+	if (digest == NULL)
+	{
+		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid message digest");
+		return FAILURE;
+	}
+
+	/* Sorts the CRL */
+	X509_CRL_sort(res->crl);
+
+	/* Check whether to use version 2 crls */
+	if(crlv2 || res->forcev2) {
+		if (!X509_CRL_set_version(res->crl, 1)) {
+			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Failed to use v2 CRLs");
+			return FAILURE;
+		}
+	}
+	
+	
+	if(!crlv2 && res->forcev2) {
+			php_error_docref(NULL TSRMLS_CC, E_NOTICE, "The CRL contains extensions which need a V2 CRL, creating a V2 CRL");
+	}
+	
+	capkey = php_openssl_evp_from_zval(zcapkey, 0, capass, capass_len, 0, &capkeyres TSRMLS_CC);
+
+	if(!capkey) {
+		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Failed to read CA private key");
+		return FAILURE;
+	}
+
+	/* verify private key */
+	if (!X509_check_private_key(res->cacert,capkey)) {
+		php_error_docref(NULL TSRMLS_CC, E_WARNING, "CA Cert does not match private key");
+		return FAILURE;
+	}
+
+	/* sign CRL */
+	if (!X509_CRL_sign(res->crl,capkey,digest)) {
+		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Failed to sign CRL");
+		return FAILURE;
+	}
+	
+	return SUCCESS;
+}
+
+/* {{{ proto bool openssl_crl_export(resource crl, string &data, mixed capkey[, bool crlv2[, bool notext[, string capass]]])
+   Exports a CRL to a string */
+PHP_FUNCTION(openssl_crl_export) 
+{
+	BIO *out;
+	BUF_MEM *buf;
+	
+	zval *crl_res;
+	struct php_x509_crl *res = NULL;
+		
+	zval *zcapkey;
+	
+	char *capass;
+	long capass_len;
+
+	int crlv2 = 1;
+	int notext = 1;
+	
+	long digest_alg = OPENSSL_ALGO_SHA1;
+	
+	zval *output;
+	
+	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rz/z|bbsl",
+							&crl_res, &output,		
+							&zcapkey,
+							&crlv2, &notext,
+							&capass, &capass_len,
+							&digest_alg) == FAILURE) {
+		RETURN_NULL();
+	}
+
+	if ((res = (struct php_x509_crl *)zend_fetch_resource(Z_RES_P(crl_res), "OpenSSL X.509 CRL", le_crl)) == NULL) {
+		RETURN_FALSE;
+	}
+	
+	if(php_openssl_crl_generate(res, zcapkey, crlv2, capass, capass_len, digest_alg TSRMLS_CC) != SUCCESS) {
+		RETURN_FALSE;
+	}
+
+	
+	out = BIO_new(BIO_s_mem());
+	if (!notext) {
+		X509_CRL_print(out, res->crl);
+	}
+	
+	if (!PEM_write_bio_X509_CRL(out, res->crl))  {
+		BIO_free(out);
+		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Failed to export CRL");
+		RETURN_FALSE;
+	}
+
+	zval_dtor(output);
+	BIO_get_mem_ptr(out, &buf);
+	ZVAL_STRINGL(output, buf->data, buf->length);
+	
+	BIO_free(out);
+	
+	RETURN_TRUE;
+	
+}
+/* }}} */
+ 
+/* {{{ proto bool openssl_crl_export_file(resource crl, string filename, mixed capkey[, bool crlv2[, bool notext[, string capass]]])
+   Exports a CRL to a file */
+PHP_FUNCTION(openssl_crl_export_file)
+{
+	BIO *out;
+	
+	zval *crl_res;
+	struct php_x509_crl *res = NULL;
+	
+	char *export_filename;
+	long export_filename_len;
+
+	zval *zcapkey;
+	
+	char *capass;
+	long capass_len;
+
+	int crlv2 = 0;
+	int notext = 1;
+	
+	long digest_alg = OPENSSL_ALGO_SHA1;
+
+	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rsz|bbsl",
+							&crl_res,		
+							&export_filename, &export_filename_len,
+							&zcapkey,
+							&crlv2, &notext,
+							&capass, &capass_len, 
+							&digest_alg) == FAILURE) {
+		RETURN_NULL();
+	}
+
+	if ((res = (struct php_x509_crl *)zend_fetch_resource(Z_RES_P(crl_res), "OpenSSL X.509 CRL", le_crl)) == NULL) {
+		RETURN_FALSE;
+	}
+	
+	if(php_openssl_crl_generate(res, zcapkey, crlv2, capass, capass_len, digest_alg TSRMLS_CC) != SUCCESS) {
+		RETURN_FALSE;
+	}
+
+
+	/* write CRL */
+	if(php_openssl_open_base_dir_chk(export_filename TSRMLS_CC) != 0) {
+		RETURN_FALSE;
+	}
+
+	out = BIO_new(BIO_s_file());
+	
+	if(BIO_write_filename(out, export_filename) <= 0) {
+		BIO_free(out);
+		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Failed to write CRL");
+		RETURN_FALSE;
+	}
+	if (!notext) {
+		X509_CRL_print(out, res->crl);
+	}
+	
+	PEM_write_bio_X509_CRL(out,res->crl);
+	BIO_free(out);
+
+	RETURN_TRUE;
 }
 /* }}} */
