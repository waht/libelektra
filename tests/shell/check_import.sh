@INCLUDE_COMMON@

echo
echo ELEKTRA IMPORT SCRIPTS TESTS
echo

check_version


ROOT=$USER_ROOT
FILE=`mktemp`
SIDE=$ROOT/../side_val
PLUGIN=simpleini
DATADIR=@CMAKE_CURRENT_BINARY_DIR@/data

cleanup()
{
	rm -f $FILE
}


echo "Import with existing root"

$KDB set $ROOT "root" >/dev/null
exit_if_fail "could not set root"

test `$KDB ls $ROOT` = $ROOT
succeed_if "Root key not found"

$KDB import $ROOT simpleini < $DATADIR/one_value.simpleini
succeed_if "Could not run kdb import"

test "`$KDB ls $ROOT`" = "user/tests/script"
succeed_if "key name not correct"

test "`$KDB get $ROOT`" = root
succeed_if "root value not correct"

$KDB export $ROOT simpleini > $FILE
succeed_if "Could not run kdb export"

diff $DATADIR/one_value.simpleini $FILE
succeed_if "Export file one_value.simpleini was not equal"

$KDB rm -r $ROOT
succeed_if "Could not remove root"



echo "Import with empty root"

$KDB import $ROOT simpleini < $DATADIR/one_value.simpleini
succeed_if "Could not run kdb import"

test "`$KDB ls $ROOT`" = "user/tests/script"
succeed_if "key name not correct"

test "`$KDB get $ROOT`" = root
succeed_if "root value not correct"

$KDB export $ROOT simpleini > $FILE
succeed_if "Could not run kdb export"

diff $DATADIR/one_value.simpleini $FILE
succeed_if "Export file one_value.simpleini was not equal"



echo "Import with wrong root (overwrite)"

$KDB set $SIDE val
succeed_if "Could not set $SIDE"

$KDB set $ROOT "wrong_root" >/dev/null
exit_if_fail "could not set wrong_root"

$KDB import -s overwrite $ROOT simpleini < $DATADIR/one_value.simpleini
succeed_if "Could not run kdb import"

test "`$KDB ls $ROOT`" = "user/tests/script"
succeed_if "key name not correct"

test "`$KDB get $ROOT`" = root
succeed_if "root value not correct"

$KDB export $ROOT simpleini > $FILE
succeed_if "Could not run kdb export"

diff $DATADIR/one_value.simpleini $FILE
succeed_if "Export file one_value.simpleini was not equal"

$KDB rm -r $ROOT
succeed_if "Could not remove root"

test "`$KDB get $SIDE`" = val
succeed_if "root value not correct"

$KDB rm $SIDE
succeed_if "Could not remove $SIDE"





echo "Import two values"

$KDB import $ROOT simpleini < $DATADIR/two_value.simpleini
succeed_if "Could not run kdb import"

test "`$KDB ls $ROOT`" = "user/tests/script
user/tests/script/key"
succeed_if "key name not correct"

test "`$KDB get $ROOT`" = root
succeed_if "root value not correct"

test "`$KDB get $ROOT/key`" = value
succeed_if "key value not correct"

$KDB export $ROOT simpleini > $FILE
succeed_if "Could not run kdb export"

diff $DATADIR/two_value.simpleini $FILE
succeed_if "Export file two_value.simpleini was not equal"



echo "Import one value (cut two values from previous test case)"

$KDB set $SIDE val
succeed_if "Could not set $SIDE"

$KDB import -s cut $ROOT simpleini < $DATADIR/one_value.simpleini
succeed_if "Could not run kdb import"

test "`$KDB ls $ROOT`" = "user/tests/script"
succeed_if "key name not correct"

test "`$KDB get $ROOT`" = root
succeed_if "root value not correct"

$KDB export $ROOT simpleini > $FILE
succeed_if "Could not run kdb export"

diff $DATADIR/one_value.simpleini $FILE
succeed_if "Export file one_value.simpleini was not equal"

test "`$KDB get $SIDE`" = val
succeed_if "root value not correct"

$KDB rm $SIDE
succeed_if "Could not remove $SIDE"





echo "Import one value (cut previous value)"

$KDB set $ROOT wrong_root
succeed_if "Could not set $ROOT"

$KDB set $ROOT/val wrong_val
succeed_if "Could not set $ROOT/val"

$KDB set $SIDE val
succeed_if "Could not set $SIDE"

$KDB import -s cut $ROOT simpleini < $DATADIR/one_value.simpleini
succeed_if "Could not run kdb import"

test "`$KDB ls $ROOT`" = "user/tests/script"
succeed_if "key name not correct"

test "`$KDB get $ROOT`" = root
succeed_if "root value not correct"

$KDB export $ROOT simpleini > $FILE
succeed_if "Could not run kdb export"

diff $DATADIR/one_value.simpleini $FILE
succeed_if "Export file one_value.simpleini was not equal"

test "`$KDB get $SIDE`" = val
succeed_if "root value not correct"

$KDB rm $SIDE
succeed_if "Could not remove $SIDE"


$KDB rm -r $ROOT
succeed_if "Could not remove $ROOT"

end_script
