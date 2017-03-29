var myLayout;

$(document).ready( function() {

  r2ui.load_colors();
  r2.load_settings();

  // Create panels
  var disasm_panel = new DisasmPanel();
  disasm_panel.display = "graph";
  r2ui._dis = disasm_panel;

  // For enyo compatibility
  r2ui.ra = {};
  r2ui.mp = {};
  r2ui.ra.getIndex = function() {};
  r2ui.ra.setIndex = function() {};
  r2ui.mp.openPage = function() {};
  r2ui._hex = {};
  r2ui._hex.seek = function() {};


  // Workaround for Chrome 48 getTransformToElement removal
  // https://github.com/clientIO/joint/issues/203
  SVGElement.prototype.getTransformToElement = SVGElement.prototype.getTransformToElement || function(toElement) {
    return toElement.getScreenCTM().inverse().multiply(this.getScreenCTM());
  };

  // Render Disasm Panel
  r2ui._dis.seek(location.hash.substring(1));
  r2ui._dis.render("graph");
});

// Overwritting since its used in disasm_panel.js
function render_history() {}

function scroll_to_element(element) {
  if (element === undefined || element === null) return;
  var top = Math.max(0,element.documentOffsetTop() - ( window.innerHeight / 2 ));
  $('#center_panel').scrollTo(top, {axis: 'y'});
  r2ui._dis.scroll_offset = top;
}

function store_scroll_offset() {
  r2ui._dis.scroll_offset = $('#center_panel').scrollTop();
}
function scroll_to_last_offset() {
  if (r2ui._dis.scroll_offset !== null) $('#center_panel').scrollTo(r2ui._dis.scroll_offset, {axis: 'y'});
}
