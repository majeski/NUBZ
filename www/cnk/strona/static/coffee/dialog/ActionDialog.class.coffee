root = exports ? this
root.ActionDialog = class ActionDialog extends root.QuestionDialog
  _prepareDialog: (dialog) =>
    super
    instance = this
    jQuery("input[type=text]", dialog)
      .each( ->
        jQuery(this).parent().next().css("color", instance._data.utils.style.inputErrorColor)
        jQuery(this).keyup( (e) ->
          obj = jQuery(this)
          regex = new RegExp(instance._data.utils.regex.input)
          error = obj.parent().next()
          if not obj.val().match regex
              if obj.val().length
                instance._showInputError(error, instance._data.utils.text.inputError)
              else
                instance._showInputError(error, instance._data.utils.text.emptyInputError)
          else
            obj.parent().next().html("")
        )
      )

    return

  _prepareFilledDialog: (dialog) =>
    jQuery(".form-group:eq(0) input", dialog).val(@_dialogInfo.text)
    if @readonly
      jQuery("input", dialog).prop("readonly", true)
      @_dialog.getButton('saveButtonDialog').hide()
    @

  extractData: =>
    text = jQuery("#dialog input").val()
    data =
      text: text
    data
