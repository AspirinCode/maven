#include "point.h"
EicPoint::EicPoint(float x, float y, Peak* peak, MainWindow* mw)
{

	setFlag(QGraphicsItem::ItemIsFocusable);
    setFlag(QGraphicsItem::ItemIsSelectable);
    setAcceptsHoverEvents(true);
    _x = x;
    _y = y;
    _mw = mw;
    _peak = peak;

	//mouse press events
    connect(this, SIGNAL(peakSelected(Peak*)), mw, SLOT(showPeakInfo(Peak*)));
    connect(this, SIGNAL(peakSelected(Peak*)), mw->getEicWidget(), SLOT(showPeakArea(Peak*)));
	//mouse hover events
    connect(this, SIGNAL(peakGroupFocus(PeakGroup*)), mw->getEicWidget(), SLOT(setSelectedGroup(PeakGroup*)));
    connect(this, SIGNAL(peakGroupFocus(PeakGroup*)), mw->getEicWidget()->scene(), SLOT(update()));


}

EicPoint::~EicPoint() {}

QRectF EicPoint::boundingRect() const
{
	float cSize = 10 + 30*(_peak->quality);
	return(QRectF(_x-cSize/2,_y-cSize/2,cSize,cSize));
}

void EicPoint::hoverEnterEvent (QGraphicsSceneHoverEvent*) {
	this->setFocus(Qt::MouseFocusReason);

	//update colors of all peaks belonging to this group
       foreach (QGraphicsItem *item, scene()->items()) {
       if (_group && qgraphicsitem_cast<EicPoint *>(item)) {
           if (((EicPoint*) item)->getPeakGroup() == _group) item->update();
       }
    }

	string sampleName;
    if (_peak && _peak->getSample() ) { sampleName = _peak->getSample()->sampleName; }
	setToolTip( 		"<b>  Sample: </b>"   + QString( sampleName.c_str() ) + 
						"<br> <b>intensity: </b>" +   QString::number(_peak->peakIntensity) +
						"<br> <b>area: </b>" + 		  QString::number(_peak->peakAreaCorrected) +
						"<br> <b>rt: </b>" +   QString::number(_peak->rt, 'f', 2 ) +
						"<br> <b>scan#: </b>" +   QString::number(_peak->scan ) +
						"<br> <b>m/z: </b>" + QString::number(_peak->peakMz, 'f', 6 )
						/*
						"<br> <b>quality:  </b>"  + QString::number(_peak->quality, 'f', 2) +
						"<br> <b>sigma:  </b>"  + QString::number(_peak->gaussFitSigma, 'f', 2) +
						"<br> <b>fitR2:  </b>"  + QString::number(_peak->gaussFitR2*100, 'f', 2)
						"<br> <b>Group Overlap Frac: </b>" + QString::number(_peak->groupOverlapFrac)
						"<br> <b>peakAreaFractional: </b>" + QString::number(_peak->peakAreaFractional) +
						"<br> <b>noNoiseFraction: </b>" + QString::number(_peak->noNoiseFraction) +
						"<br> <b>symmetry: </b>" + QString::number(_peak->symmetry) +
						"<br> <b>sigma: </b>" + QString::number(_peak->gaussFitSigma) +
						"<br> <b>r2: </b>" + QString::number(_peak->gaussFitR2) +
						"<br> <b>angle: </b>" + QString::number(_peak->angle) +
						"<br> <b>rank: </b>" + QString::number(_peak->peakRank) +
						"<br> <b>S/N: </b>" + QString::number(_peak->signalBaselineRatio, 'f', 4) +
						"<br> <b>Width: </b>" + QString::number(_peak->width) +
						"<br> <b>No NoiseObs: </b>" + QString::number(_peak->noNoiseObs) +
						"<br> <b>Group Overlap Frac: </b>" + QString::number(_peak->groupOverlapFrac) +
						*/
		);
	update();

	if( _group != NULL) { _group->isFocused = true; emit(peakGroupFocus(_group)); }
}

void EicPoint::hoverLeaveEvent ( QGraphicsSceneHoverEvent*) {
	 clearFocus();
	 if (_group) _group->isFocused = false;

    foreach (QGraphicsItem *item, scene()->items()) {
       if (qgraphicsitem_cast<EicPoint *>(item)) {
           if (((EicPoint*) item)->getPeakGroup() == _group) item->update();
       }
    }
    update(); 
}

void EicPoint::mouseDoubleClickEvent(QGraphicsSceneMouseEvent*) {
   bookmark();
}


void EicPoint::mousePressEvent (QGraphicsSceneMouseEvent* event) {
    //if (_group) _group->groupOveralMatrix();

    if (event->button() == Qt::RightButton)  {
        contextMenuEvent(event);
        return;
    }

    setZValue(10);
    if (_group) emit peakGroupSelected(_group);
    if( _peak)  emit peakSelected(_peak);

    if ( _group && _group->isIsotope() == false ) {
        _mw->isotopeWidget->setPeakGroup(_group);
    }

    //if (_peak && _mw->spectraWidget->isVisible()) {
    //    _mw->spectraWidget->setScan(_peak);
    //}

    if (_peak && _mw->covariantsPanel->isVisible()) {
        _mw->getLinks(_peak);
    }

    if (_peak && _mw->adductWidget->isVisible()) {
        _mw->adductWidget->setPeak(_peak);
    }

    scene()->update();
}



void EicPoint::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    QPen pen = _pen;
    QBrush brush = _brush;

    int maxRadius = scene()->height()*0.05;
    if (maxRadius > 30) maxRadius = 30; 
    if (maxRadius < 5) maxRadius=5;

    PeakGroup* selGroup = _mw->getEicWidget()->getSelectedGroup();

    if (_group != NULL && selGroup == _group ) {
        brush.setStyle(Qt::SolidPattern);
        pen.setColor(_color.darker());
        pen.setWidth(_pen.width()+1);
    } else {
        brush.setStyle(Qt::NoBrush);
    }

    painter->setPen(pen);
    painter->setBrush(brush);
    float cSize = 6 + maxRadius*(_peak->quality);
    painter->drawEllipse(_x-cSize/2, _y-cSize/2, cSize,cSize);
}


void EicPoint::setClipboardToGroup() { _mw->setClipboardToGroup(_group); }
void EicPoint::bookmark() { _mw->bookmarkPeakGroup(_group); }

void EicPoint::setClipboardToIsotopes() {
    if (_group != NULL &&_group->compound != NULL && ! _group->compound->formula.empty() )  {
        _mw->isotopeWidget->setPeakGroup(_group);
    }
}

void EicPoint::linkCompound() {
	if (_group != NULL &&_group->compound != NULL )  {
            //link group to compound
            _group->compound->setPeakGroup(*_group);

            //update compound retention time
            _group->compound->expectedRt=_peak->rt;

            //log information about retention time change
           // _mw->getEicWidget()->addNote(_peak->peakMz,_peak->peakIntensity, "Compound Link");
            _mw->getEicWidget()->saveRetentionTime();

            //upadte ligand widget
            QString dbname(_group->compound->db.c_str());
            _mw->ligandWidget->setDatabaseAltered(dbname,true);
            //_mw->ligandWidget->updateTable();
            _mw->ligandWidget->updateCurrentItemData();

            //update pathway widget with new concentration information
            _mw->pathwayWidget->updateCompoundConcentrations();
	}
}




void EicPoint::reorderSamples() { if (_mw && _group ) _mw->reorderSamples(_group ); }

void EicPoint::contextMenuEvent ( QGraphicsSceneMouseEvent* event ) {
    QMenu menu;



    QAction* c1 = menu.addAction("Copy Details to Clipboard");
    c1->setIcon(QIcon(rsrcPath + "/copyCSV.png"));
    connect(c1, SIGNAL(triggered()), SLOT(setClipboardToGroup()));

    if (_group && _group->compound ) {
       if ( _group->isIsotope() == false && !_group->compound->formula.empty() ) {
            QAction* z = menu.addAction("Copy Isotope Information to Clipboard");
            z->setIcon(QIcon(rsrcPath + "/copyCSV.png"));
            connect(z, SIGNAL(triggered()), SLOT(setClipboardToIsotopes()));
        }

        QAction* e = menu.addAction("Link to Compound");
        e->setIcon(QIcon(rsrcPath + "/link.png"));
        connect(e, SIGNAL(triggered()), SLOT(linkCompound()));
    }

    QAction* c2 = menu.addAction("Mark Good");
    c2->setIcon(QIcon(rsrcPath + "/markgood.png"));
    connect(c2, SIGNAL(triggered()), _mw->getEicWidget(), SLOT(markGroupGood()));

    QAction* c3 = menu.addAction("Mark Bad");
    c3->setIcon(QIcon(rsrcPath + "/markbad.png"));
    connect(c3, SIGNAL(triggered()), _mw->getEicWidget(), SLOT(markGroupBad()));

    if ( _group && _group->peaks.size() > 1  ) {
        QAction* d = menu.addAction("Sort Samples by Peak Intensity");
        connect(d, SIGNAL(triggered()), SLOT(reorderSamples()));
    }

    QAction *selectedAction = menu.exec(event->screenPos());


    //event->ignore();
}

void EicPoint::keyPressEvent( QKeyEvent *e ) {
	bool marked=false;
	if (!_group) return;

	//qDebug() << "Point::keyPressEvent() " << e->key() << endl;

	switch( e->key() ) {
	}
	update();
	e->accept();
}
