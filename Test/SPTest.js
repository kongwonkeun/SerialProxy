//
var logger;
var host = "ws://localhost:5331/write_anything_here.php";
var socket;
var buffer = 0;
var count = 0;

$(document).ready(function()
{
    logger = document.getElementById("datalog");

    function connect() {
        try {
            socket = new WebSocket(host);
            log('socket create = ' + socket.readyState);

            socket.onopen = function() {
                log('socket open = ' + socket.readyState);
            }

            socket.onmessage = function(event) {
                count++;
                var msg = new String(event.data);
                logger.innerHTML  = "RX " + count + " = ";
                logger.innerHTML += msg + "<br/>";
            }

            socket.onclose = function() {
                log('socket close = ' + socket.readyState);
                socket.close();
            }
        } catch(e) {
            log('error = ' + e);
        }
    }

    function send() {
        var text = $('#text').val();
        if (text == "") return;
        try {
            socket.send(text);
            log('TX = ' + text);
            $('#text').val("");
        } catch(e) {
            log('connection problem = ' + e);
        }
    }

    function log(msg) { logger.innerHTML += msg + "<br/>"; }

    $('#disconnect').click(function() { log("proxy disconnecting ..."); socket.close(); });	
    $('#connect').click(function() { log("proxy connecting ..."); connect(); });
    $('#text').keypress(function(event) { if (event.keyCode == '13') send(); });	
    $('#send').click(function() { send(); });
});

$(window).load(function()
{
    $('#connect').click();
});
//