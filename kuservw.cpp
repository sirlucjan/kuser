#include "kuservw.moc"

KUserView::KUserView(QWidget *parent, const char *name) : QWidget( parent, name )
{
  init();
}

KUserView::~KUserView()
{
  delete m_Header;
  delete m_Users;
}

void KUserView::clear() {
  m_Users->clear();
}

void KUserView::insertItem(KUser *aku) {
  m_Users->insertItem(aku);
}

int KUserView::currentItem() {
  return (m_Users->currentItem());
}

void KUserView::setCurrentItem(int item) {
  m_Users->setCurrentItem(item);
}

void KUserView::init()
{
  m_Header = new KHeader(this, "_header", 2, KHeader::Resizable|KHeader::Buttons );
  m_Header->setGeometry(2, 0, width(), 0 );

  m_Users = new KUserTable(this, "_table" );
  m_Users->setGeometry(0, m_Header->height(), width(), height()-m_Header->height() );

  m_Header->setHeaderLabel(0, "Login");
  m_Header->setHeaderLabel(1, "Full Name");

  connect(m_Users, SIGNAL(highlighted(int,int)), SLOT(onHighlight(int,int)));
  connect(m_Users, SIGNAL(selected(int,int)), SLOT(onSelect(int,int)));
  connect(m_Header, SIGNAL(sizeChanged(int,int)), m_Users, SLOT(setColumnWidth(int,int)));

// This connection makes it jumpy and slow (but it works!)
//	connect(m_Header, SIGNAL(sizeChanging(int,int)), m_UserList, SLOT(setColumnWidth(int,int)));

  connect(m_Users, SIGNAL(hSliderMoved(int)), m_Header, SLOT(setOrigin(int)));

  m_Header->setHeaderSize(0, 80);
  m_Header->setHeaderSize(1, 280);
}

void KUserView::onSelect(int row, int)
{
  emit selected(row);
}

void KUserView::onHighlight(int row, int)
{
  emit highlighted(row);
}

void KUserView::resizeEvent(QResizeEvent *rev)
{
  m_Header->resize(rev->size().width(), 0);
  m_Users->setGeometry(0, m_Header->height(), rev->size().width(), rev->size().height()-m_Header->height());
}
