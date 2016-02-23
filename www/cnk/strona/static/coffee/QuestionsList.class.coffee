root = exports ? this
root.QuestionsList = class QuestionsList extends root.View
  constructor: (@_containerId) ->
    super
    @_questions = []
    @_init()

  _init: =>
    jQuery.ajaxSetup(
      headers: { "X-CSRFToken": getCookie("csrftoken") }
    )
    jQuery.ajax(
      type: 'POST'
      context: this
      url: '/getQuestionsList/'
      success: (data) =>
        jQuery(data).appendTo(@_containerId)
        @_getQuestionsListElementHTML()
    )

  _getQuestionsListElementHTML: =>
    jQuery.ajaxSetup(
      headers: { "X-CSRFToken": getCookie("csrftoken") }
    )
    jQuery.ajax(
      type: 'POST'
      context: this
      url: '/getQuestionsListElement/'
      success: (data) ->
        @_questionsListElementHTML = data
    )
    return

  addQuestions: (questions) =>
    for id, q of questions
      questionListElement = jQuery(@_questionsListElementHTML)
      questionName = jQuery(".questionNameCell > div", questionListElement)
      questionBadge = jQuery(".questionBadge", questionListElement)
      questionListElement.data("name", questionName)
        .data("badge", questionBadge)
      questionName.html "#{q.name}"
      questionBadge.html q.type
      jQuery(questionListElement).click( do (id) => ( =>
        @fireEvents("questionClicked", id)))
      @_questions.push questionListElement
    @_refreshQuestionsList()

  _refreshQuestionsList: =>
    jQuery("#questionsList .questionsListElement").each( -> jQuery(this).remove())
    for q in @_questions
      question = q.clone(true, true)
      jQuery(question).appendTo("#questionsListTable > tbody")
      jQuery(".questionNameCell > div", question).shortenText()
    return
