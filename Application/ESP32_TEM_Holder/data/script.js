
/** Global variables **/ 

// Placeholder/Position where notifications are placed
var notification_place = 'notification_setting_position';

// Notifications to be sent 
var notification_msg_setting_position = '<div class="notification is-warning" id="notification_msg_setting_position"> <h3> Setting Holder Position ... </h3><progress class="progress is-medium is-dark" max="100">45%</progress></div>';
var notification_id_setting_position = 'notification_msg_setting_position';

var notification_msg_position_set = '<div class="notification is-success" id="notification_msg_position_set"><h3 class="has-text-white"> Holder Position set!</h3></div>';
var notification_id_position_set = 'notification_msg_position_set';

var notification_msg_ws_diconnect = '<div class="notification is-danger" id="notification_msg_ws_diconnect"><h3>Server got disconnected! Please refresh the page...</h3></div>';
var notification_id_ws_diconnect = 'notification_msg_ws_diconnect';

var notification_msg_calibration = '<div class="notification is-warning" id="notification_msg_calibration"><h3>You have entered calibration mode!</h3> <h4>Be careful, you could damage the holder irreversibly</h4></div>';
var notification_id_calibration = 'notification_msg_calibration';

/** Javascript Functions **/

//slider functionality
function slider_change(){

  var slider = document.getElementById("sliderWithValue");
  var output = document.getElementById('value');

  var probe_pos_1 = document.getElementById("Probe_Pos_1");
  var probe_pos_2 = document.getElementById("Probe_Pos_2");
  var probe_pos_3 = document.getElementById("Probe_Pos_3");

  var pos_1 = parseInt(slider.min) + 50;
  var pos_2 = (slider.max - slider.min)/2;
  var pos_3 = slider.max - 50;

  output.innerHTML = slider.value;

  if(slider.value > (pos_3+pos_2)/2){
    probe_pos_3.checked = true;
  } else if(slider.value < ((pos_1+pos_2)/2)){
    probe_pos_1.checked = true;
  } else {
    probe_pos_2.checked = true;
  }
  arrow_position();

}

// check boxes functionality
function set_probe_pos(id){

  var slider = document.getElementById("sliderWithValue");
  var output = document.getElementById('value');

  var pos_1 = parseInt(slider.min) + 50;
  var pos_2 = (slider.max - slider.min)/2;
  var pos_3 = slider.max - 50;

  var position;
  switch (id) {
    case "Probe_Pos_1":
      position = pos_1;
      break;
    case "Probe_Pos_2":
      position = pos_2;
      break;
    case "Probe_Pos_3":
      position = pos_3;
      break;
  }
  slider.value = position;
  output.innerHTML = position;
  arrow_position();
}


// moves the arrow to the correct position
function arrow_position(){
var slider = document.getElementById("sliderWithValue");
var pos_2 = (slider.max - slider.min)/2;
var pos_arrow = document.getElementById("arrow_position");
pos_arrow.style.left = (parseFloat(slider.value)-pos_2)*0.15 + '%';
}

// plus//minus button on slider functionality
function slider_plus_minus_btn(plus_minus) {
  var slider = document.getElementById("sliderWithValue");
  
  switch (plus_minus) {
    case 'plus':
      slider.value = parseInt(slider.value) + parseInt(slider.step);
      break;
    case 'minus':
      slider.value = parseInt(slider.value) - parseInt(slider.step);
      break;
    default:
      break;
  }

  slider_change();
}

//Sets holder position, sends toast/notification and sends desired holder position to backend
function set_holder_position_btn(){
  var button = document.getElementById('button_holder_set');
  var value = document.getElementById('sliderWithValue').value; // slider value

  button.className += " is-loading";
  addNotification(notification_msg_setting_position);

  var Tx_Json = { "message_type" : "POSITION", 
                  "data" : value};
  
  console.log(value);
  console.log("TX: " + JSON.stringify(Tx_Json));

  webSocket.send(JSON.stringify(Tx_Json)); // send slider value to backend

  Tx_Json = undefined;

}

// adds notification to the status panel
function addNotification(message) {
  let div = document.getElementById(notification_place);
  div.innerHTML += message;
}

// function addNotification(id, msg, msg_id){
//   var myStyle = document.getElementById('myStyle').sheet;
//   myStyle.insertRule('#'+msg_id+'{transition: opacity 0.5s;}');
//   newdiv = document.getElementById(id);
//   newdiv.innerHTML += msg;
//   document.getElementById("notification").setAttribute("id", msg_id);
// }

function removeNotification(msg_id){
  let newdiv = document.getElementById(msg_id);
  if(newdiv != null){
    newdiv.remove();
  }
}

//** switch functionality */ 
// change to Live View Mode
function switch_Live_Mode(){
var switch_Live_Mode = document.getElementById('switch_Live_Mode');
var button_holder_set = document.getElementById('button_holder_set');

  if(switch_Live_Mode.checked == true){
    addNotification(notification_msg_calibration);
    document.body.style.backgroundColor = 'grey';

    // TODO: send message to backend
  } else {
    removeNotification(notification_id_calibration);
    document.body.style.backgroundColor = 'white';
  }

}

// change to Precision Mode
function switch_Precision_Mode(){
  var switch_Precision_Mode = document.getElementById('switch_Precision_Mode');
  var slider = document.getElementById("sliderWithValue");
  switch_Precision_Mode.oninput = function(){
    if(switch_Precision_Mode.checked == true){
      slider.step= 1;
    } else {
      slider.step= 10;
    }
  }
}


// Position Handler: Receives current position on all clients
function position_handler(data){
  var slider = document.getElementById("sliderWithValue");
  slider.value = data;
  slider_change();
}

// Acknowledgement Handler: Checks if Position is set
function ack_handler(data){

  switch (data) {
    case "SET":
      removeNotification(notification_id_setting_position);
      var button = document.getElementById('button_holder_set');
      button.classList.remove("is-loading");

      addNotification(notification_msg_position_set);
      setTimeout(function(){
        removeNotification(notification_id_position_set);
        }, 3000);
      break;
    case "ERROR":
      console.log("Something went WRONG");
    default:
      break;
  }
}

// Error Handler: Informs user about errors in the backend
function error_handler(data){
  switch (data) {
    case "ERROR_1":
      console.log("Error 1 received!");
      break;
    case "ERROR_2":
      console.log("Error 2 received!");
      break;
    default:
      break;
  }
}

// When a new message from websocket is received they get sorted here
function message_handler(event){

  console.log("RX: " + event.data);

  const json = JSON.parse(event.data);

  switch (json.message_type) {
    case 'POSITION':
        position_handler(json.data);
      break;
    case 'ACK':
        ack_handler(json.data);
      break;
    case 'STATUS':
        error_handler(json.data);
      break;
    default:
      break;
  }
    
}
