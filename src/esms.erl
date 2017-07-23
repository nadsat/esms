-module(esms).
-on_load(init/0).
%% API exports
-export([decode/1,encode/2]).


-define(APPNAME, esms).
-define(LIBNAME, esms).

%%====================================================================
%% API functions
%%====================================================================

decode (_)->
  not_loaded(?LINE).

encode (_,_)->
  not_loaded(?LINE).

%%====================================================================
%% Internal functions
%%====================================================================
init() ->
    SoName = case code:priv_dir(?APPNAME) of
        {error, bad_name} ->
            case filelib:is_dir(filename:join(["..", priv])) of
                true ->
                    filename:join(["..", priv, ?LIBNAME]);
                _ ->
                    filename:join([priv, ?LIBNAME])
            end;
        Dir ->
            filename:join(Dir, ?LIBNAME)
    end,
    erlang:load_nif(SoName, 0).

not_loaded(Line) ->
    exit({not_loaded, [{module, ?MODULE}, {line, Line}]}).
