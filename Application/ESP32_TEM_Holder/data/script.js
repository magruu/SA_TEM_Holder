
/** Global variables **/ 

// Notifications to be sent 
var notification_msg_setting_position = '<div class="notification is-warning" id="notification"> <h3> Setting Holder Position ... </h3><progress class="progress is-medium is-dark" max="100">45%</progress></div>';
var notification_msg_position_set = '<div class="notification is-success" id="notification"><h3 class="has-text-white"> Holder Position set!</h3></div>';

// Holder Position Status
var position_set = false;


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

  if(this.value > (pos_3+pos_2)/2){
    probe_pos_3.checked = true;
  } else if(this.value < ((pos_1+pos_2)/2)){
    probe_pos_1.checked = true;
  } else {
    probe_pos_2.checked = true;
  }
  arrow_position();

}

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


// moves the arrow to the right position
function arrow_position(){
var slider = document.getElementById("sliderWithValue");
var pos_2 = (slider.max - slider.min)/2;
var pos_arrow = document.getElementById("arrow_position");
pos_arrow.style.left = (parseFloat(slider.value)-pos_2)*0.15 + '%';
}

// plus//minus button on slider functionality
function slider_plus_minus_btn(plus_minus) {
  var slider = document.getElementById("sliderWithValue");
  var output = document.getElementById('value');

  var operator;

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
  output.innerHTML = slider.value;
  arrow_position();
}

//holder position and send toast/notification
function set_holder_position_btn(){
  var button = document.getElementById('button_holder_set');

  button.className += " is-loading";
  addNotifcation('notification_setting_position', notification_msg_setting_position);
  var value = document.getElementById('sliderWithValue').value;
  console.log(value);
  webSocket.send(value);

  if(position_set == true){
    addNotifcation('notification_setting_position', notification_msg_position_set);
  }

  setTimeout(function(){button.classList.remove("is-loading");},4500);

}

// add notification
function addNotifcation(id, msg) {
  var notification_setting = document.getElementById(id);
  const  div = document.getElementById(id);
  div.innerHTML += msg;

  newdiv = document.getElementById("notification");
  setTimeout(function(){
   newdiv.style.opacity = '0';
   newdiv.addEventListener('transitionend', () => newdiv.remove());

  }, 4000);
}

// switch functionality
// change to Live View Mode
function switch_Live_Mode(){
var switch_Live_Mode = document.getElementById('switch_Live_Mode');
var button_holder_set = document.getElementById('button_holder_set');

switch_Live_Mode.oninput = function(){
  if(switch_Live_Mode.checked == true){
    button_holder_set.style.visibility = "hidden";
  } else {
    button_holder_set.style.visibility = "visible";
  }
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


