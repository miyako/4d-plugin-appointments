//%attributes = {}
$calendars:=APPOINTMENT NAMES   //calendar list

$appointments:=ALL APPOINTMENTS ("名称未設定 2")

$appointment:=Get appointment ("BB126029-86FB-43A3-A29C-CBD7FD690799")

$event:=New object:C1471
$event.title:="testtesttest"
$event.startDate:=String:C10(Current date:C33;ISO date GMT:K1:10;Current time:C178)
$event.endDate:=String:C10(Current date:C33;ISO date GMT:K1:10;Current time:C178)
$event.notes:="aaa"
$event.location:="診療録No"

$status:=CREATE APPOINTMENT ("名称未設定 2";$event)
$status:=UPDATE APPOINTMENT ($status.event)
$status:=DELETE APPOINTMENT ($status.event)
