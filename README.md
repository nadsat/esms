esms
=====
 Erlang lib for encoding/decoding SMS in PDU mode. It's built using nif based on https://github.com/kkroo/smspdu-cpp

Usage
-----

```erlang
esms:decode("079183600300008006070C9183903113476111103091244280111030912482800000").
```
getting as a result:

```
{ok,[{payload,"Discharge timestamp: 11-01-03 19:42:28\nMessage ID: 7\nStatus: Ok, short message received by the SME"},
     {smsc,"380630000008"},
     {sender,"380913317416"},
     {date,"11-01-03"},
     {time,"19:42:24"},
     {udh_type,[]},
     {udh_data,[]}]}
```

```erlang
esms:encode("This is an example",[{smsc,"1234"},{number,"5678"},{alphabet,"binary"}]).
```

```
{ok,"039121431100049165870004AA125468697320697320616E206578616D706C65"}
```
Supported values for alphabet are : "binary","iso","gsm" and "ucs2"

Build
-----

    $ rebar3 compile
