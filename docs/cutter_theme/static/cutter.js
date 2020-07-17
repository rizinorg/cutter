
// Make sure that external links are opened in new tabs
$(document).ready(function() {
    $("a[href^='http']").attr('target','_blank');
});