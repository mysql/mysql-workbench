#ifndef _SQLIDE_MAIN_H_
#define _SQLIDE_MAIN_H_

void setup_sqlide(wb::WBContextUI *wbui, 
                  std::string &name, 
                  sigc::slot<FormViewBase*, boost::shared_ptr<bec::UIForm> > &slot);

#endif /* _SQLIDE_MAIN_H_ */
