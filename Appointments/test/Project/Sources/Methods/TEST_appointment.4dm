//%attributes = {}
$event:=New object:C1471
$event.title:="testtesttest"
$event.startDate:=String:C10(Current date:C33;ISO date GMT:K1:10;Current time:C178)
$event.endDate:=String:C10(Current date:C33;ISO date GMT:K1:10;Current time:C178)
$event.notes:="aaa"
$event.duration:=3600
$event.location:="診療録No"

$j:=JSON Stringify:C1217($event)
CREATE APPOINTMENT ("名称未設定 2";$j;$e)
