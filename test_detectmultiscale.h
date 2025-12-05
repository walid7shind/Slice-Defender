#ifndef TEST_DETECTMULTISCALE_H
#define TEST_DETECTMULTISCALE_H

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/core/types.hpp>

#include <vector>
#include <string>
#include <stdexcept>

/*
 * Classe PalmDetector :
 * - Capture le flux vidéo depuis la cam
 * - Détecte le centre de la paume (contours + fallback cascade)
 */
class PalmDetector {
public:
    /**
     * Initialise la capture et charge le cascade classifier.
     * @param deviceId    index de la caméra OpenCV (ex. 0 pour la webcam)
     * @param cascadePath chemin vers le XML du cascade palm
     * @throws runtime_error si échec d’ouverture cam ou de chargement cascade
     */
    PalmDetector(int deviceId, const std::string& cascadePath);

    /**
     * Récupère une frame, détecte la paume et annote l’image.
     * @param frame   frame BGR en entrée/sortie (dessin rectangles & cercles)
     * @param centers liste des points centraux détectés (coord. pixel)
     * @return true si la frame est capturée et traitée, false sinon
     */
    bool getAnnotatedFrame(cv::Mat& frame, std::vector<cv::Point>& centers);

private:
    //=== Ressources internes OpenCV ============================
    cv::VideoCapture     cap;          ///< périphérique vidéo
    cv::CascadeClassifier palmCascade; ///< fallback cascade classifier
    cv::Ptr<cv::CLAHE>   claheCr;      ///< CLAHE canal Cr
    cv::Ptr<cv::CLAHE>   claheCb;      ///< CLAHE canal Cb
};

#endif // TEST_DETECTMULTISCALE_H
