extern "C" {

#include "pdu.h"
#include "erl_nif.h"
#include <string.h>

const unsigned int MAX_MSG_SIZE = 1024;
const unsigned int ANS_LEN = 7;
const unsigned int MAX_ATOM = 512;
const unsigned int MAX_FIELD = 512;
const char* GEN_ERROR = "default_error";

static ERL_NIF_TERM
mk_atom(ErlNifEnv* env, const char* atom)
{
    ERL_NIF_TERM ret;

    if(!enif_make_existing_atom(env, atom, &ret, ERL_NIF_LATIN1))
    {
        return enif_make_atom(env, atom);
    }

    return ret;
}

static ERL_NIF_TERM
mk_error(ErlNifEnv* env, const char* mesg)
{
  ERL_NIF_TERM t;
  t = enif_make_string (env,mesg,ERL_NIF_LATIN1);
  return enif_make_tuple2(env, mk_atom(env, "error"), t);
}

static PDU::Alphabet 
getAlphabet (const char* al){
  //set to default
  PDU::Alphabet ret = PDU::UCS2;

  if ( strcmp("gsm",al) == 0 ){
    ret = PDU::GSM;
  }   
  else if ( strcmp("iso",al) == 0 ){
    ret = PDU::ISO;
  }
  else if ( strcmp("binary",al) == 0 ){
    ret = PDU::BINARY;
  }
  else if ( strcmp("ucs2",al) == 0 ){
    ret = PDU::UCS2;
  }
  return ret;
}

static ERL_NIF_TERM decode_nif(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[])
{
  unsigned int length;
  char * msg = NULL;
  const char * err_str = GEN_ERROR;
  ERL_NIF_TERM *ans = NULL;
  ErlNifBinary bin;
  ERL_NIF_TERM res;
  ERL_NIF_TERM t;
  int isb = 0;
  PDU *pdu = NULL;

  if (enif_inspect_binary(env, argv[0], &bin)) {
    length = bin.size;
    isb    = 1;
  }
  else {
    if (!enif_get_list_length(env, argv[0], &length)) {
      err_str = "string_length";
      goto DERR;
    }
  }
  if (length > MAX_MSG_SIZE ){
    err_str = "pdu_too_long";
    goto DERR;
  } 

  msg = (char *)malloc(length + 1);

  if (msg == NULL) {
    err_str = "memory_allocation";
    goto DERR;
  }
  
  if (isb){
    memcpy(msg,(const char*)bin.data,length);
    msg[length] = '\0';
  } 
  else {
    if (enif_get_string(env, argv[0], msg, length+1, ERL_NIF_LATIN1) < 1) {
      err_str = "getting_string";
      goto DERR;
    }
  }

  pdu = new PDU(msg);
  if ( pdu == NULL) {
    err_str = "pdu_allocation";
    goto DERR;
  }

  if (!pdu->parse()){
      err_str =  pdu->getError();
      goto DERR;
  }

  ans = (ERL_NIF_TERM*)enif_alloc(ANS_LEN*sizeof(ERL_NIF_TERM)); 

  t = enif_make_string (env,pdu->getMessage(),ERL_NIF_LATIN1);
  ans[0] =  enif_make_tuple2 (env,mk_atom(env,"payload"), t);

  t = enif_make_string (env,pdu->getSMSC(),ERL_NIF_LATIN1);
  ans[1] =  enif_make_tuple2 (env,mk_atom(env,"smsc"), t);

  t = enif_make_string (env,pdu->getNumber(),ERL_NIF_LATIN1);
  ans[2] =  enif_make_tuple2 (env,mk_atom(env,"sender"), t);

  t = enif_make_string (env,pdu->getDate(),ERL_NIF_LATIN1);
  ans[3] =  enif_make_tuple2 (env,mk_atom(env,"date"), t);

  t = enif_make_string (env,pdu->getTime(),ERL_NIF_LATIN1);
  ans[4] =  enif_make_tuple2 (env,mk_atom(env,"time"), t);

  t = enif_make_string (env,pdu->getUDHType(),ERL_NIF_LATIN1);
  ans[5] =  enif_make_tuple2 (env,mk_atom(env,"udh_type"), t);

  t = enif_make_string (env,pdu->getUDHData(),ERL_NIF_LATIN1);
  ans[6] =  enif_make_tuple2 (env,mk_atom(env,"udh_data"), t);

  res = enif_make_list_from_array(env, ans, ANS_LEN);

  free(msg);
  delete pdu;
  enif_free(ans);
  
  return enif_make_tuple2 (env,mk_atom(env, "ok"),res);

DERR:
    res = mk_error(env, err_str);
    if (msg){
      free(msg);
    }
    if (pdu){
      delete pdu;
    }
    return res;
}

static ERL_NIF_TERM encode_nif(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[]){
  const char * err_str = GEN_ERROR;
  unsigned int length;
  PDU pdu;
  char * msg = NULL;
  char atom[MAX_ATOM];
  char field[MAX_FIELD];
  ERL_NIF_TERM opts,val;
  const ERL_NIF_TERM *array;
  ERL_NIF_TERM res;
  int arity;
  PDU::Alphabet alphabet;

  if (!enif_get_list_length(env, argv[0], &length)) {
    err_str = "string_length";
    goto EERR;
  }
  if (length > MAX_MSG_SIZE ){
    err_str = "msg_too_long";
    goto EERR;
  } 

  msg = (char *)malloc(length + 1);

  if (msg == NULL) {
    err_str = "memory_allocation";
    goto EERR;
  }

  if (enif_get_string(env, argv[0], msg, length+1, ERL_NIF_LATIN1) < 1) {
    err_str = "setting_msg";
    goto EERR;
  }
  pdu.setMessage(msg);

  opts = argv[1];

  if(!enif_is_list(env, opts)) {
    err_str = "options_not_list";
    goto EERR;
  }
  //default values
  pdu.setSMSC("123");
  pdu.setNumber("123");
  pdu.setAlphabet (PDU::UCS2);

  while(enif_get_list_cell(env, opts, &val, &opts)) {
    if (!enif_get_tuple(env,val,&arity,&array)){
      err_str = "option_tuple";
      goto EERR;
    }
    if (!enif_get_atom(env,array[0],atom,MAX_ATOM,ERL_NIF_LATIN1)){
      err_str = "option_atom";
      goto EERR;
    }
    if (enif_get_string(env, array[1], field, length+1, ERL_NIF_LATIN1) < 1) {
      err_str = "option_field";
      goto EERR;
    }

    if ( strcmp("smsc",atom) == 0 ){
      pdu.setSMSC(field);
    }
    else if ( strcmp("number",atom) == 0 ){ 
      pdu.setNumber(field);
    }
    else if ( strcmp("alphabet",atom) == 0 ){ 
      alphabet = getAlphabet (field);
      pdu.setAlphabet (alphabet);
    }
  }
  pdu.generate();
  res = enif_make_string (env,pdu.getPDU(),ERL_NIF_LATIN1);

  free(msg);

  return enif_make_tuple2 (env,mk_atom(env, "ok"),res);

EERR:
    if (msg){
      free(msg);
    }
    return mk_error(env, err_str);
}

static ErlNifFunc nif_funcs[] = {
    {"decode", 1, decode_nif},
    {"encode", 2, encode_nif}
};

ERL_NIF_INIT(esms, nif_funcs, NULL, NULL, NULL, NULL)

}
