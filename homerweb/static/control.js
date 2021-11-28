/* eslint-env jquery */

function setup_fastctrl() {
  if ($('input#state')) {
    $('#fastctrl').append('<input id="faston" value="State On" type="button">');
    $('#fastctrl').on('click', '#faston', function() {
      $('#state').val('on');
      $('#submit_state').trigger('click');
    });
    $('#fastctrl').append('<input id="fastoff" value="State Off" type="button">');
    $('#fastctrl').on('click', '#fastoff', function() {
      $('#state').val('off');
      $('#submit_state').trigger('click');
    });
  }
  if ($('input#brightness')) {
    $('#fastslider').slider({
      min: 0,
      max: 255,
      value: $('#brightness').val(),
      orientation: "horizontal",
      slide: function(event, ui) {
        $("#brightness").val(ui.value);
      }
    });
  }
}

$(window).on('load', setup_fastctrl);
